/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2021 Craig Edwards and D++ contributors
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This folder is a modified fork of libdave, https://github.com/discord/libdave
 * Copyright (c) 2024 Discord, Licensed under MIT
 *
 ************************************************************************************/
#include "session.h"
#include <thread>
#include <vector>
#include <cstring>
#include <iostream>
#include <mls/crypto.h>
#include <mls/messages.h>
#include <dpp/export.h>
#include <dpp/snowflake.h>
#include <mls/state.h>
#include <dpp/cluster.h>
#include "mls_key_ratchet.h"
#include "user_credential.h"
#include "parameters.h"
#include "persisted_key_pair.h"
#include "util.h"
#include "openssl/evp.h"

#define TRACK_MLS_ERROR(reason)				\
	if (failure_callback) {				\
		failure_callback(__FUNCTION__, reason);	\
	}

namespace dpp::dave::mls {

struct queued_proposal {
	::mlspp::ValidatedContent content;
	::mlspp::bytes_ns::bytes ref;
};

session::session(dpp::cluster& cluster, key_pair_context_type context, dpp::snowflake auth_session_id, mls_failure_callback callback) noexcept
  : signing_key_id(auth_session_id), key_pair_context(context), failure_callback(std::move(callback)), creator(cluster)
{
	creator.log(dpp::ll_debug, "Creating a new MLS session");
}

session::~session() noexcept = default;

void session::init(protocol_version version, dpp::snowflake group_id, dpp::snowflake self_user_id, std::shared_ptr<::mlspp::SignaturePrivateKey>& transient_key) noexcept {
	reset();

	bot_user_id = self_user_id;

	creator.log(dpp::ll_debug, "Initializing MLS session with protocol version " + std::to_string(version) + " and group ID " + group_id.str());
	session_protocol_version = version;
	session_group_id = std::move(big_endian_bytes_from(group_id).as_vec());

	init_leaf_node(self_user_id, transient_key);

	create_pending_group();
}

void session::reset() noexcept {
	creator.log(dpp::ll_debug, "Resetting MLS session");

	clear_pending_state();

	current_state.reset();
	outbound_cached_group_state.reset();

	session_protocol_version = 0;
	session_group_id.clear();
}

void session::set_protocol_version(protocol_version version) noexcept {
	if (version != session_protocol_version) {
		// when we need to retain backwards compatibility
		// there may be some changes to the MLS objects required here
		// until then we can just update the stored version
		session_protocol_version = version;
	}
}

std::vector<uint8_t> session::get_last_epoch_authenticator() const noexcept {
	if (!current_state) {
		creator.log(dpp::ll_debug, "Cannot get epoch authenticator without an established MLS group");
		return {};
	}
	return std::move(current_state->epoch_authenticator().as_vec());
}

void session::set_external_sender(const std::vector<uint8_t> &external_sender_package) noexcept
try {
	if (current_state) {
		creator.log(dpp::ll_warning, "Cannot set external sender after joining/creating an MLS group");
		return;
	}

	creator.log(dpp::ll_debug, "Unmarshalling MLS external sender");

	mls_external_sender = std::make_unique<::mlspp::ExternalSender>(
	  ::mlspp::tls::get<::mlspp::ExternalSender>(external_sender_package));

	if (!session_group_id.empty()) {
		create_pending_group();
	}
}
catch (const std::exception& e) {
	creator.log(dpp::ll_error, "Failed to unmarshal external sender: " + std::string(e.what()));
	TRACK_MLS_ERROR(e.what());
	return;
}

std::optional<std::vector<uint8_t>> session::process_proposals(std::vector<uint8_t> proposals, std::set<dpp::snowflake> const& recognised_user_ids) noexcept
try {
	if (!pending_group_state && !current_state) {
		creator.log(dpp::ll_debug, "Cannot process proposals without any pending or established MLS group state");
		return std::nullopt;
	}

	if (!state_with_proposals) {
		state_with_proposals = std::make_unique<::mlspp::State>(
			pending_group_state ? *pending_group_state : *current_state);
	}

	creator.log(dpp::ll_debug, "Processing MLS proposals message of " + std::to_string(proposals.size()) + " bytes");

	::mlspp::tls::istream in_stream(proposals);

	bool is_revoke = false;
	in_stream >> is_revoke;

	if (is_revoke) {
		creator.log(dpp::ll_trace, "Revoking from proposals");
	}

	const auto suite = state_with_proposals->cipher_suite();

	if (is_revoke) {
		std::vector<::mlspp::bytes_ns::bytes> refs;
		in_stream >> refs;

		for (const auto& ref : refs) {
			bool found = false;
			for (auto it = proposal_queue.begin(); it != proposal_queue.end(); it++) {
				if (it->ref == ref) {
					found = true;
					proposal_queue.erase(it);
					break;
				}
			}

			if (!found) {
				creator.log(dpp::ll_debug, "Cannot revoke unrecognized proposal ref");
				TRACK_MLS_ERROR("Unrecognized proposal revocation");
				return std::nullopt;
			}
		}

		state_with_proposals = std::make_unique<::mlspp::State>(
			pending_group_state ? *pending_group_state : *current_state);

		for (auto& prop : proposal_queue) {
			// success will queue the proposal, failure will throw
			state_with_proposals->handle(prop.content);
		}
	} else {
		std::vector<::mlspp::MLSMessage> messages;
		in_stream >> messages;

		for (const auto& proposal_message : messages) {
			auto validated_content = state_with_proposals->unwrap(proposal_message);

			if (!validate_proposal_message(validated_content.authenticated_content(), *state_with_proposals, recognised_user_ids)) {
				return std::nullopt;
			}

			// success will queue the proposal, failure will throw
			state_with_proposals->handle(validated_content);

			auto ref = suite.ref(validated_content.authenticated_content());

			proposal_queue.push_back({
			  std::move(validated_content),
			  std::move(ref),
			});
		}
	}

	// generate a commit
	auto commit_secret = ::mlspp::hpke::random_bytes(suite.secret_size());

	auto commit_options = ::mlspp::CommitOpts{
	  {},		// no extra proposals
	  true,		// inline tree in welcome
	  false,	// do not force path
	  {}		// default leaf node options
	};

	auto [commit_message, welcome_message, new_state] = state_with_proposals->commit(commit_secret, commit_options, {});

	creator.log(dpp::ll_debug, "Prepared commit/welcome/next state for MLS group from received proposals");

	// combine the commit and welcome messages into a single buffer
	auto out_stream = ::mlspp::tls::ostream();
	out_stream << commit_message;

	// keep a copy of the commit, we can check incoming pending group commit later for a match
	pending_group_commit = std::make_unique<::mlspp::MLSMessage>(std::move(commit_message));

	// if there were any add proposals in this commit, then we also include the welcome message
	if (welcome_message.secrets.size() > 0) {
		out_stream << welcome_message;
	}

	// cache the outbound state in case we're the winning sender
	outbound_cached_group_state = std::make_unique<::mlspp::State>(std::move(new_state));

	return out_stream.bytes();
}
catch (const std::exception& e) {
	creator.log(dpp::ll_warning, "Failed to parse MLS proposals: " + std::string(e.what()));
	TRACK_MLS_ERROR(e.what());
	return std::nullopt;
}

bool session::is_recognized_user_id(const ::mlspp::Credential& cred, std::set<dpp::snowflake> const& recognised_user_ids) const
{
	dpp::snowflake uid(user_credential_to_string(cred, session_protocol_version));
	if (uid.empty()) {
		creator.log(dpp::ll_warning, "Attempted to verify credential of unexpected type");
		return false;
	}

	if (recognised_user_ids.find(uid) == recognised_user_ids.end()) {
		creator.log(dpp::ll_warning, "Attempted to verify credential for unrecognized user ID: " + std::to_string(uid));
		return false;
	}

	return true;
}

bool session::validate_proposal_message(::mlspp::AuthenticatedContent const& message, ::mlspp::State const& target_state, std::set<dpp::snowflake> const& recognised_user_ids) const {
	if (message.wire_format != ::mlspp::WireFormat::mls_public_message) {
		creator.log(dpp::ll_warning, "MLS proposal message must be PublicMessage");
		TRACK_MLS_ERROR("Invalid proposal wire format");
		return false;
	}

	if (message.content.epoch != target_state.epoch()) {
		creator.log(dpp::ll_warning, "MLS proposal message must be for current epoch (" + std::to_string(message.content.epoch) + " != " + std::to_string(target_state.epoch()) + ")");
		TRACK_MLS_ERROR("Proposal epoch mismatch");
		return false;
	}

	if (message.content.content_type() != ::mlspp::ContentType::proposal) {
		creator.log(dpp::ll_warning, "process_proposals called with non-proposal message");
		TRACK_MLS_ERROR("Unexpected message type");
		return false;
	}

	if (message.content.sender.sender_type() != ::mlspp::SenderType::external) {
		creator.log(dpp::ll_warning, "MLS proposal must be from external sender");
		TRACK_MLS_ERROR("Unexpected proposal sender type");
		return false;
	}

	const auto& proposal = ::mlspp::tls::var::get<::mlspp::Proposal>(message.content.content);
	switch (proposal.proposal_type()) {
	case ::mlspp::ProposalType::add: {
		const auto& credential =
		  ::mlspp::tls::var::get<::mlspp::Add>(proposal.content).key_package.leaf_node.credential;
		if (!is_recognized_user_id(credential, recognised_user_ids)) {
			creator.log(dpp::ll_warning, "MLS proposal must be for recognised user");
			TRACK_MLS_ERROR("Unexpected user ID in add proposal");
			return false;
		}
		break;
	}
	case ::mlspp::ProposalType::remove:
		// Remove proposals are always allowed (mlspp will validate that it's a recognized user)
		break;
	default:
		creator.log(dpp::ll_warning, "MLS proposal must be add or remove");
		TRACK_MLS_ERROR("Unexpected proposal type");
		return false;
	}

	return true;
}

bool session::can_process_commit(const ::mlspp::MLSMessage& commit) noexcept
{
	if (!state_with_proposals) {
		return false;
	}

	if (commit.group_id() != session_group_id) {
		creator.log(dpp::ll_warning, "MLS commit message was for unexpected group");
		return false;
	}

	return true;
}

roster_variant session::process_commit(std::vector<uint8_t> commit) noexcept
try {
	creator.log(dpp::ll_debug, "Processing commit");

	auto commit_message = ::mlspp::tls::get<::mlspp::MLSMessage>(commit);

	if (!can_process_commit(commit_message)) {
		creator.log(dpp::ll_warning, "process_commit called with unprocessable MLS commit");
		return ignored_t{};
	}

	// in case we're the sender of this commit
	// we need to pull the cached state from our outbound cache
	std::optional<::mlspp::State> optional_cached_state = std::nullopt;
	if (outbound_cached_group_state) {
		optional_cached_state = *(outbound_cached_group_state.get());
	}

	auto new_state = state_with_proposals->handle(commit_message, optional_cached_state);
	if (!new_state) {
		creator.log(dpp::ll_warning, "MLS commit handling did not produce a new state");
		return failed_t{};
	}

	creator.log(dpp::ll_debug, "Successfully processed MLS commit, updating state; our leaf index is " + std::to_string(new_state->index().val) + "; current epoch is " + std::to_string(new_state->epoch()));

	roster_map ret = replace_state(std::make_unique<::mlspp::State>(std::move(*new_state)));

	// reset the outbound cached group since we handled the commit for this epoch
	outbound_cached_group_state.reset();
	clear_pending_state();

	return ret;
}
catch (const std::exception& e) {
	creator.log(dpp::ll_warning, "Failed to process MLS commit: " + std::string(e.what()));
	TRACK_MLS_ERROR(e.what());
	return failed_t{};
}

std::optional<roster_map> session::process_welcome(std::vector<uint8_t> welcome, std::set<dpp::snowflake> const& recognised_user_ids) noexcept
try {
	if (!has_cryptographic_state_for_welcome()) {
		creator.log(dpp::ll_warning, "Missing local crypto state necessary to process MLS welcome");
		return std::nullopt;
	}

	if (!mls_external_sender) {
		creator.log(dpp::ll_warning, "Cannot process MLS welcome without an external sender");
		return std::nullopt;
	}

	if (current_state) {
		creator.log(dpp::ll_warning, "Cannot process MLS welcome after joining/creating an MLS group");
		return std::nullopt;
	}

	// unmarshal the incoming welcome
	auto unmarshalled_welcome = ::mlspp::tls::get<::mlspp::Welcome>(welcome);

	// construct the state from the unmarshalled welcome
	auto new_state = std::make_unique<::mlspp::State>(
		*join_init_private_key,
		*hpke_private_key,
		*signature_private_key,
		*join_key_package,
		unmarshalled_welcome,
		std::nullopt,
		std::map<::mlspp::bytes_ns::bytes, ::mlspp::bytes_ns::bytes>());

	// perform application-level verification of the new state
	if (!verify_welcome_state(*new_state, recognised_user_ids)) {
		creator.log(dpp::ll_warning, "Group received in MLS welcome is not valid");
		return std::nullopt;
	}

	creator.log(dpp::ll_debug, "Successfully welcomed to MLS Group, our leaf index is " + std::to_string(new_state->index().val) + "; current epoch is " + std::to_string(new_state->epoch()));

	// make the verified state our new (and only) state
	roster_map ret = replace_state(std::move(new_state));

	// clear out any pending state for creating/joining a group
	clear_pending_state();

	return ret;
}
catch (const std::exception& e) {
	creator.log(dpp::ll_warning, "Failed to create group state from MLS welcome: " + std::string(e.what()));
	TRACK_MLS_ERROR(e.what());
	return std::nullopt;
}

roster_map session::replace_state(std::unique_ptr<::mlspp::State>&& state)
{
	roster_map new_roster;
	for (const ::mlspp::LeafNode& node : state->roster()) {
		if (node.credential.type() != ::mlspp::CredentialType::basic) {
			continue;
		}

		const auto& cred = node.credential.template get<::mlspp::BasicCredential>();
		new_roster[from_big_endian_bytes(cred.identity)] = node.signature_key.data.as_vec();
	}

	roster_map change_map;

	std::set_difference(new_roster.begin(), new_roster.end(), roster.begin(), roster.end(), std::inserter(change_map, change_map.end()));

	struct missing_item_wrapper {
		roster_map& map;

		using iterator = roster_map::iterator;
		using const_iterator = roster_map::const_iterator;
		using value_type = roster_map::value_type;

		iterator insert(const_iterator it, const value_type& value)
		{
			return map.try_emplace(it, value.first, std::vector<uint8_t>{});
		}

		iterator begin() { return map.begin(); }

		iterator end() { return map.end(); }
	};

	missing_item_wrapper wrapper{change_map};

	std::set_difference(roster.begin(),
			    roster.end(),
			    new_roster.begin(),
			    new_roster.end(),
			    std::inserter(wrapper, wrapper.end()));

	roster = std::move(new_roster);
	current_state = std::move(state);

	return change_map;
}

bool session::has_cryptographic_state_for_welcome() const noexcept
{
	return join_key_package && join_init_private_key && signature_private_key && hpke_private_key;
}

bool session::verify_welcome_state(::mlspp::State const& state, std::set<dpp::snowflake> const& recognised_user_ids) const
{
	if (!mls_external_sender) {
		creator.log(dpp::ll_warning, "Cannot verify MLS welcome without an external sender");
		TRACK_MLS_ERROR("Missing external sender when processing Welcome");
		return false;
	}

	auto ext = state.extensions().template find<mlspp::ExternalSendersExtension>();
	if (!ext) {
		creator.log(dpp::ll_warning, "MLS welcome missing external senders extension");
		TRACK_MLS_ERROR("Welcome message missing external sender extension");
		return false;
	}

	if (ext->senders.size() != 1) {
		creator.log(dpp::ll_warning, "MLS welcome lists unexpected number of external senders: " + std::to_string(ext->senders.size()));
		TRACK_MLS_ERROR("Welcome message lists unexpected external sender count");
		return false;
	}

	if (ext->senders.front() != *mls_external_sender) {
		creator.log(dpp::ll_warning, "MLS welcome lists unexpected external sender");
		TRACK_MLS_ERROR("Welcome message lists unexpected external sender");
		return false;
	}

	// TODO: Until we leverage revocation in the protocol
	// if we re-enable this change we will refuse welcome messages
	// because someone was previously supposed to be added but disconnected
	// before all in-flight proposals were handled.

	for (const auto& leaf : state.roster()) {
		if (!is_recognized_user_id(leaf.credential, recognised_user_ids)) {
			creator.log(dpp::ll_warning, "MLS welcome lists unrecognized user ID");
		}
	}

	return true;
}

void session::init_leaf_node(dpp::snowflake self_user_id, std::shared_ptr<::mlspp::SignaturePrivateKey>& transient_key) noexcept
try {
	auto ciphersuite = ciphersuite_for_protocol_version(session_protocol_version);

	if (!transient_key) {
		if (!signing_key_id.empty()) {
			transient_key = get_persisted_key_pair(creator, key_pair_context, signing_key_id.str(), session_protocol_version);
			if (!transient_key) {
				creator.log(dpp::ll_warning, "Did not receive MLS signature private key from get_persisted_key_pair; aborting");
				return;
			}
		}
		else {
			transient_key = std::make_shared<::mlspp::SignaturePrivateKey>(
			  ::mlspp::SignaturePrivateKey::generate(ciphersuite));
		}
	}

	signature_private_key = transient_key;

	auto self_credential = create_user_credential(self_user_id.str(), session_protocol_version);
	hpke_private_key = std::make_unique<::mlspp::HPKEPrivateKey>(::mlspp::HPKEPrivateKey::generate(ciphersuite));
	self_leaf_node = std::make_unique<::mlspp::LeafNode>(
		ciphersuite, hpke_private_key->public_key, signature_private_key->public_key, std::move(self_credential),
		leaf_node_capabilities_for_protocol_version(session_protocol_version), ::mlspp::Lifetime::create_default(),
		leaf_node_extensions_for_protocol_version(session_protocol_version), *signature_private_key
	);

	creator.log(dpp::ll_debug, "Created MLS leaf node");
}
catch (const std::exception& e) {
	creator.log(dpp::ll_warning, "Failed to initialize MLS leaf node: " + std::string(e.what()));
	TRACK_MLS_ERROR(e.what());
}

void session::reset_join_key_package() noexcept
try {
	if (!self_leaf_node) {
		creator.log(dpp::ll_warning, "Cannot initialize join key package without a leaf node");
		return;
	}

	auto ciphersuite = ciphersuite_for_protocol_version(session_protocol_version);
	join_init_private_key = std::make_unique<::mlspp::HPKEPrivateKey>(::mlspp::HPKEPrivateKey::generate(ciphersuite));
	join_key_package = std::make_unique<::mlspp::KeyPackage>(ciphersuite, join_init_private_key->public_key, *self_leaf_node, leaf_node_extensions_for_protocol_version(session_protocol_version), *signature_private_key);
}
catch (const std::exception& e) {
	creator.log(dpp::ll_warning, "Failed to initialize join key package: " + std::string(e.what()));
	TRACK_MLS_ERROR(e.what());
}

void session::create_pending_group() noexcept
try {
	if (session_group_id.empty()) {
		creator.log(dpp::ll_warning, "Cannot create MLS group without a group ID");
		return;
	}

	if (!mls_external_sender) {
		creator.log(dpp::ll_debug, "Cannot create MLS group without external sender");
		return;
	}

	if (!self_leaf_node) {
		creator.log(dpp::ll_warning, "Cannot create MLS group without self leaf node");
		return;
	}

	creator.log(dpp::ll_debug, "Creating a pending MLS group");

	auto ciphersuite = ciphersuite_for_protocol_version(session_protocol_version);
	pending_group_state = std::make_unique<::mlspp::State>(
		session_group_id,
		ciphersuite,
		*hpke_private_key,
		*signature_private_key,
		*self_leaf_node,
		group_extensions_for_protocol_version(session_protocol_version, *mls_external_sender)
	);
	creator.log(dpp::ll_debug, "Created a pending MLS group");
}
catch (const std::exception& e) {
	creator.log(dpp::ll_warning, "Failed to create MLS group: " + std::string(e.what()));
	TRACK_MLS_ERROR(e.what());
	return;
}

std::vector<uint8_t> session::get_marshalled_key_package() noexcept
try {
	// key packages are not meant to be re-used
	// so every time the client asks for a key package we create a new one
	reset_join_key_package();

	if (!join_key_package) {
		creator.log(dpp::ll_warning, "Cannot marshal an uninitialized key package");
		return {};
	}

	return ::mlspp::tls::marshal(*join_key_package);
}
catch (const std::exception& e) {
	creator.log(dpp::ll_warning, "Failed to marshal join key package: " + std::string(e.what()));
	TRACK_MLS_ERROR(e.what());
	return {};
}

std::unique_ptr<key_ratchet_interface> session::get_key_ratchet(dpp::snowflake user_id) const noexcept
{
	if (!current_state) {
		creator.log(dpp::ll_warning, "Cannot get key ratchet without an established MLS group");
		return nullptr;
	}

	// change the string user ID to a little endian 64 bit user ID
	// TODO: Make this use dpp::snowflake
	uint64_t u64_user_id = user_id;
	auto user_id_bytes = ::mlspp::bytes_ns::bytes(sizeof(u64_user_id));
	memcpy(user_id_bytes.data(), &u64_user_id, sizeof(u64_user_id));

	// generate the base secret for the hash ratchet
	auto secret = current_state->do_export(session::USER_MEDIA_KEY_BASE_LABEL, user_id_bytes, AES_GCM_128_KEY_BYTES);

	// this assumes the MLS ciphersuite produces an AES_GCM_128_KEY_BYTES sized key
	// would need to be updated to a different ciphersuite if there's a future mismatch
	return std::make_unique<mls_key_ratchet>(creator, current_state->cipher_suite(), std::move(secret));
}

void session::get_pairwise_fingerprint(uint16_t version, dpp::snowflake user_id, pairwise_fingerprint_callback callback) const noexcept
try {
	if (!current_state || !signature_private_key) {
		throw std::invalid_argument("No established MLS group");
	}

	uint64_t remote_user_id = user_id;
	uint64_t self_user_id = bot_user_id;

	auto it = roster.find(remote_user_id);
	if (it == roster.end()) {
		throw std::invalid_argument("Unknown user ID: " + std::to_string(user_id));
	}

	::mlspp::tls::ostream toHash1;
	::mlspp::tls::ostream toHash2;

	toHash1 << version;
	toHash1.write_raw(it->second);
	toHash1 << remote_user_id;

	toHash2 << version;
	toHash2.write_raw(signature_private_key->public_key.data);
	toHash2 << self_user_id;

	std::vector<std::vector<uint8_t>> keyData = {
	  toHash1.bytes(),
	  toHash2.bytes(),
	};

	std::sort(keyData.begin(), keyData.end());

	std::thread([callback = std::move(callback),
				 data = ::mlspp::bytes_ns::bytes(std::move(keyData[0])) + keyData[1]] {
		static constexpr uint8_t salt[] = {
		  0x24,
		  0xca,
		  0xb1,
		  0x7a,
		  0x7a,
		  0xf8,
		  0xec,
		  0x2b,
		  0x82,
		  0xb4,
		  0x12,
		  0xb9,
		  0x2d,
		  0xab,
		  0x19,
		  0x2e,
		};

		constexpr uint64_t N = 16384, r = 8, p = 2, max_mem = 32 * 1024 * 1024;
		constexpr size_t hash_len = 64;

		std::vector<uint8_t> out(hash_len);

		int ret = EVP_PBE_scrypt((const char*)data.data(), data.size(), salt, sizeof(salt), N, r, p, max_mem, out.data(), out.size());

		if (ret == 1) {
			callback(out);
		} else {
			callback({});
		}
	}).detach();
}
catch (const std::exception& e) {
	creator.log(dpp::ll_warning, "Failed to generate pairwise fingerprint: " + std::string(e.what()));
	callback({});
}

void session::clear_pending_state()
{
	pending_group_state.reset();
	pending_group_commit.reset();
	join_init_private_key.reset();
	join_key_package.reset();
	hpke_private_key.reset();
	self_leaf_node.reset();
	state_with_proposals.reset();
	proposal_queue.clear();
}

}
