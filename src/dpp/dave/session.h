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
} // namespace mlspp

namespace dpp::dave::mls {

struct QueuedProposal;

class session {
public:
	using mls_failure_callback = std::function<void(std::string const&, std::string const&)>;

	session(key_pair_context_type context,
		const std::string& authSessionId,
		mls_failure_callback callback) noexcept;

	~session() noexcept;

	void init(protocol_version version,
		  uint64_t groupId,
		  std::string const& selfUserId,
		  std::shared_ptr<::mlspp::SignaturePrivateKey>& transientKey) noexcept;
	void reset() noexcept;

	void set_protocol_version(protocol_version version) noexcept;
	[[nodiscard]] protocol_version get_protocol_version() const noexcept { return protocolVersion_; }

	[[nodiscard]] std::vector<uint8_t> get_last_epoch_authenticator() const noexcept;

	void set_external_sender(std::vector<uint8_t> const& externalSenderPackage) noexcept;

	std::optional<std::vector<uint8_t>> process_proposals(
	  std::vector<uint8_t> proposals,
	  std::set<std::string> const& recognizedUserIDs) noexcept;

	roster_variant process_commit(std::vector<uint8_t> commit) noexcept;

	std::optional<roster_map> process_welcome(
	  std::vector<uint8_t> welcome,
	  std::set<std::string> const& recognizedUserIDs) noexcept;

	std::vector<uint8_t> get_marshalled_key_package() noexcept;

	[[nodiscard]] std::unique_ptr<key_ratchet_interface> get_key_ratchet(std::string const& userId) const noexcept;

	using pairwise_fingerprint_callback = std::function<void(std::vector<uint8_t> const&)>;

	void get_pairwise_fingerprint(uint16_t version,
				      std::string const& userId,
				      pairwise_fingerprint_callback callback) const noexcept;

private:
	void init_leaf_node(std::string const& selfUserId,
			    std::shared_ptr<::mlspp::SignaturePrivateKey>& transientKey) noexcept;
	void reset_join_key_package() noexcept;

	void create_pending_group() noexcept;

	bool has_cryptographic_state_for_welcome() const noexcept;

	bool is_recognized_user_id(const ::mlspp::Credential& cred,
				   std::set<std::string> const& recognizedUserIDs) const;
	bool validate_proposal_message(::mlspp::AuthenticatedContent const& message,
				       ::mlspp::State const& targetState,
				       std::set<std::string> const& recognizedUserIDs) const;
	bool verify_welcome_state(::mlspp::State const& state,
				  std::set<std::string> const& recognizedUserIDs) const;

	bool can_process_commit(const ::mlspp::MLSMessage& commit) noexcept;

	roster_map replace_state(std::unique_ptr<::mlspp::State>&& state);

	void clear_pending_state();

	inline static const std::string USER_MEDIA_KEY_BASE_LABEL = "Discord Secure Frames v0";

	protocol_version protocolVersion_;
	std::vector<uint8_t> groupId_;
	std::string signingKeyId_;
	std::string selfUserId_;
	key_pair_context_type keyPairContext_{nullptr};

	std::unique_ptr<::mlspp::LeafNode> selfLeafNode_;
	std::shared_ptr<::mlspp::SignaturePrivateKey> selfSigPrivateKey_;
	std::unique_ptr<::mlspp::HPKEPrivateKey> selfHPKEPrivateKey_;

	std::unique_ptr<::mlspp::HPKEPrivateKey> joinInitPrivateKey_;
	std::unique_ptr<::mlspp::KeyPackage> joinKeyPackage_;

	std::unique_ptr<::mlspp::ExternalSender> externalSender_;

	std::unique_ptr<::mlspp::State> pendingGroupState_;
	std::unique_ptr<::mlspp::MLSMessage> pendingGroupCommit_;

	std::unique_ptr<::mlspp::State> outboundCachedGroupState_;

	std::unique_ptr<::mlspp::State> currentState_;
	roster_map roster_;

	std::unique_ptr<::mlspp::State> stateWithProposals_;
	std::list<QueuedProposal> proposalQueue_;

	mls_failure_callback onMLSFailureCallback_{};
};

} // namespace dpp::dave::mls


