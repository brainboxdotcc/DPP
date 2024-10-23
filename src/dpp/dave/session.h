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
#pragma once

#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <dpp/export.h>
#include <dpp/snowflake.h>
#include "persisted_key_pair.h"
#include "key_ratchet.h"
#include "version.h"

namespace mlspp {
	struct AuthenticatedContent;
	struct Credential;
	struct ExternalSender;
	struct HPKEPrivateKey;
	struct KeyPackage;
	struct LeafNode;
	struct MLSMessage;
	struct SignaturePrivateKey;
	class State;
}

namespace dpp {
	class cluster;
}

namespace dpp::dave::mls {

struct queued_proposal;

/**
 * @brief Represents an MLS DAVE session
 */
class session { // NOLINT
public:
	/**
	 * @brief An MLS failure callback
	 */
	using mls_failure_callback = std::function<void(std::string const&, std::string const&)>;

	/**
	 * @brief Constructor
	 * @param context key pair context (set to nullptr to use a transient key pair)
	 * @param auth_session_id auth session id (set to empty string to use a transient key pair)
	 * @param callback callback for failure
	 */
	session(dpp::cluster& cluster, key_pair_context_type context, dpp::snowflake auth_session_id, mls_failure_callback callback) noexcept;

	/**
	 * @brief Destructor
	 */
	~session() noexcept;

	/**
	 * @brief Initalise session
	 * @note This is not done in the constructor, as we may need to do this again on upgrade or downgrade,
	 * whilst preserving other state set by the constructor.
	 *
	 * @param version protocol version
	 * @param group_id group id (channel id)
	 * @param self_user_id bot's user id
	 * @param transient_key transient private key
	 */
	void init(protocol_version version, dpp::snowflake group_id, dpp::snowflake self_user_id, std::shared_ptr<::mlspp::SignaturePrivateKey>& transient_key) noexcept;

	/**
	 * @brief Reset the session to defaults
	 */
	void reset() noexcept;

	/**
	 * @brief Set protocol version for session
	 * @param version protocol version
	 */
	void set_protocol_version(protocol_version version) noexcept;

	/**
	 * @brief Get protocol version for session
	 * @return protocol version
	 */
	[[nodiscard]] protocol_version get_protocol_version() const noexcept {
		return session_protocol_version;
	}

	/**
	 * @brief Get last epoch authenticator, the discord privacy code for the vc
	 * @return privacy code
	 */
	[[nodiscard]] std::vector<uint8_t> get_last_epoch_authenticator() const noexcept;

	/**
	 * @brief Set external sender from external sender opcode
	 * @param external_sender_package external sender package
	 */
	void set_external_sender(std::vector<uint8_t> const& external_sender_package) noexcept;

	/**
	 * @brief Process proposals from proposals opcode
	 * @param proposals proposals blob from websocket
	 * @param recognised_user_ids list of recognised user IDs
	 * @return optional vector to send in reply as commit welcome
	 */
	std::optional<std::vector<uint8_t>> process_proposals(std::vector<uint8_t> proposals, std::set<dpp::snowflake> const& recognised_user_ids) noexcept;

	/**
	 * @brief Process commit message from discord websocket
	 * @param commit commit message from discord websocket
	 * @return roster list of people in the vc
	 */
	roster_variant process_commit(std::vector<uint8_t> commit) noexcept;

	/**
	 * @brief Process welcome blob
	 * @param welcome welcome blob from discord
	 * @param recognised_user_ids Recognised user ID list
	 * @return roster list of people in the vc
	 */
	std::optional<roster_map> process_welcome(std::vector<uint8_t> welcome, std::set<dpp::snowflake> const& recognised_user_ids) noexcept;

	/**
	 * @brief Get the bot user's key package for sending to websocket
	 * @return marshalled key package
	 */
	std::vector<uint8_t> get_marshalled_key_package() noexcept;

	/**
	 * @brief Get key ratchet for a user (including the bot)
	 * @param user_id User id to get ratchet for
	 * @return The user's key ratchet for use in an encryptor or decryptor
	 */
	[[nodiscard]] std::unique_ptr<key_ratchet_interface> get_key_ratchet(dpp::snowflake user_id) const noexcept;

	/**
	 * @brief callback for completion of pairwise fingerprint
	 */
	using pairwise_fingerprint_callback = std::function<void(std::vector<uint8_t> const&)>;

	/**
	 * @brief Get pairwise fingerprint (used to validate discord member in vc)
	 * @warning This uses SCRYPT and is extremely resource intensive. It will spawn a thread
	 * which will call your callback on completion.
	 * @param version Should always be 0x00
	 * @param user_id User ID to get fingerprint for
	 * @param callback Callback for completion
	 */
	void get_pairwise_fingerprint(uint16_t version, dpp::snowflake user_id, pairwise_fingerprint_callback callback) const noexcept;

private:
	/**
	 * @brief Initialise leaf node
	 * @param self_user_id Bot user id
	 * @param transient_key Transient key
	 */
	void init_leaf_node(dpp::snowflake self_user_id, std::shared_ptr<::mlspp::SignaturePrivateKey>& transient_key) noexcept;

	/**
	 * @brief Reset join key
	 */
	void reset_join_key_package() noexcept;

	/**
	 * @brief Create pending MLS group
	 */
	void create_pending_group() noexcept;

	/**
	 * @brief Is ready for welcome
	 * @return true if ready for welcome
	 */
	[[nodiscard]] bool has_cryptographic_state_for_welcome() const noexcept;

	/**
	 * @brief Check if user ID is valid
	 * @param cred MLS credential
	 * @param recognised_user_ids list of recognised user IDs
	 * @return
	 */
	[[nodiscard]] bool is_recognized_user_id(const ::mlspp::Credential& cred, std::set<dpp::snowflake> const& recognised_user_ids) const;

	/**
	 * @brief Validate proposals message
	 * @param message authenticated content message
	 * @param target_state new state
	 * @param recognised_user_ids recognised list of user IDs
	 * @return true if validated
	 */
	[[nodiscard]] bool validate_proposal_message(::mlspp::AuthenticatedContent const& message, ::mlspp::State const& target_state, std::set<dpp::snowflake> const& recognised_user_ids) const;

	/**
	 * @brief Verify that welcome state is valid
	 * @param state current state
	 * @param recognised_user_ids list of recognised user IDs
	 * @return
	 */
	[[nodiscard]] bool verify_welcome_state(::mlspp::State const& state, std::set<dpp::snowflake> const& recognised_user_ids) const;

	/**
	 * @brief Check if can process a commit now
	 * @param commit Commit message
	 * @return true if can process
	 */
	[[nodiscard]] bool can_process_commit(const ::mlspp::MLSMessage& commit) noexcept;

	/**
	 * @brief Replace state with a new one
	 * @param state new state
	 * @return new roster list of users in VC
	 */
	roster_map replace_state(std::unique_ptr<::mlspp::State>&& state);

	/**
	 * @brief Clear pending MLS state
	 */
	void clear_pending_state();

	/**
	 * @brief Constant media key label
	 */
	inline static const std::string USER_MEDIA_KEY_BASE_LABEL = "Discord Secure Frames v0";

	/**
	 * @brief DAVE protocol version for the session
	 */
	protocol_version session_protocol_version;

	/**
	 * @brief Session group ID (voice channel id)
	 */
	std::vector<uint8_t> session_group_id;

	/**
	 * @brief Signing key id
	 */
	dpp::snowflake signing_key_id;

	/**
	 * @brief The bot's user snowflake ID
	 */
	dpp::snowflake bot_user_id;

	/**
	 * @brief The bot's key pair context
	 */
	key_pair_context_type key_pair_context{nullptr};

	/**
	 * @brief Our leaf node in the ratchet tree
	 */
	std::unique_ptr<::mlspp::LeafNode> self_leaf_node;

	/**
	 * @brief The bots signature private key
	 */
	std::shared_ptr<::mlspp::SignaturePrivateKey> signature_private_key;

	/**
	 * @brief HPKE private key
	 */
	std::unique_ptr<::mlspp::HPKEPrivateKey> hpke_private_key;

	/**
	 * @brief Private key for join initialisation
	 */
	std::unique_ptr<::mlspp::HPKEPrivateKey> join_init_private_key;

	/**
	 * @brief Join key package
	 */
	std::unique_ptr<::mlspp::KeyPackage> join_key_package;

	/**
	 * @brief MLS External sender (the discord voice gateway server)
	 */
	std::unique_ptr<::mlspp::ExternalSender> mls_external_sender;

	/**
	 * @brief Pending MLS group state
	 */
	std::unique_ptr<::mlspp::State> pending_group_state;

	/**
	 * @brief Pending MLS group commit
	 */
	std::unique_ptr<::mlspp::MLSMessage> pending_group_commit;

	/**
	 * @brief Outbound cached group state
	 */
	std::unique_ptr<::mlspp::State> outbound_cached_group_state;

	/**
	 * @brief Current MLS state
	 */
	std::unique_ptr<::mlspp::State> current_state;

	/**
	 * @brief Participant roster, all users who are in the VC with dave enabled
	 */
	roster_map roster;

	/**
	 * @brief Current state containing proposals
	 */
	std::unique_ptr<::mlspp::State> state_with_proposals;

	/**
	 * @brief Queue of proposals to process
	 */
	std::list<queued_proposal> proposal_queue;

	/**
	 * @brief Function to call on failure, if any
	 */
	mls_failure_callback failure_callback{};

	/**
	 * @brief DPP Cluster, used for logging
	 */
	dpp::cluster& creator;
};

}
