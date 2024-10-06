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
	 * @param authSessionId auth session id (set to empty string to use a transient key pair)
	 * @param callback callback for failure
	 */
	session(dpp::cluster& cluster, key_pair_context_type context, const std::string& authSessionId, mls_failure_callback callback) noexcept;

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
	 * @param groupId group id (channel id)
	 * @param selfUserId bot's user id
	 * @param transientKey transient private key
	 */
	void init(protocol_version version, uint64_t groupId, std::string const& selfUserId, std::shared_ptr<::mlspp::SignaturePrivateKey>& transientKey) noexcept;

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
		return protocolVersion_;
	}

	/**
	 * @brief Get last epoch authenticator, the discord privacy code for the vc
	 * @return privacy code
	 */
	[[nodiscard]] std::vector<uint8_t> get_last_epoch_authenticator() const noexcept;

	/**
	 * @brief Set external sender from external sender opcode
	 * @param externalSenderPackage external sender package
	 */
	void set_external_sender(std::vector<uint8_t> const& externalSenderPackage) noexcept;

	/**
	 * @brief Process proposals from proposals opcode
	 * @param proposals proposals blob from websocket
	 * @param recognizedUserIDs list of recognised user IDs
	 * @return optional vector to send in reply as commit welcome
	 */
	std::optional<std::vector<uint8_t>> process_proposals(std::vector<uint8_t> proposals, std::set<std::string> const& recognizedUserIDs) noexcept;

	/**
	 * @brief Process commit message from discord websocket
	 * @param commit commit message from discord websocket
	 * @return roster list of people in the vc
	 */
	roster_variant process_commit(std::vector<uint8_t> commit) noexcept;

	/**
	 * @brief Process welcome blob
	 * @param welcome welcome blob from discord
	 * @param recognizedUserIDs Recognised user ID list
	 * @return roster list of people in the vc
	 */
	std::optional<roster_map> process_welcome(std::vector<uint8_t> welcome, std::set<std::string> const& recognizedUserIDs) noexcept;

	/**
	 * @brief Get the bot user's key package for sending to websocket
	 * @return marshalled key package
	 */
	std::vector<uint8_t> get_marshalled_key_package() noexcept;

	/**
	 * @brief Get key ratchet for a user (including the bot)
	 * @param userId User id to get ratchet for
	 * @return The user's key ratchet for use in an encryptor or decryptor
	 */
	[[nodiscard]] std::unique_ptr<key_ratchet_interface> get_key_ratchet(std::string const& userId) const noexcept;

	/**
	 * @brief callback for completion of pairwise fingerprint
	 */
	using pairwise_fingerprint_callback = std::function<void(std::vector<uint8_t> const&)>;

	/**
	 * @brief Get pairwise fingerprint (used to validate discord member in vc)
	 * @warning This uses SCRYPT and is extremely resource intensive. It will spawn a thread
	 * which will call your callback on completion.
	 * @param version Should always be 0x00
	 * @param userId User ID to get fingerprint for
	 * @param callback Callback for completion
	 */
	void get_pairwise_fingerprint(uint16_t version, std::string const& userId, pairwise_fingerprint_callback callback) const noexcept;

private:
	/**
	 * @brief Initialise leaf node
	 * @param selfUserId Bot user id
	 * @param transientKey Transient key
	 */
	void init_leaf_node(std::string const& selfUserId, std::shared_ptr<::mlspp::SignaturePrivateKey>& transientKey) noexcept;

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
	 * @param recognizedUserIDs list of recognised user IDs
	 * @return
	 */
	[[nodiscard]] bool is_recognized_user_id(const ::mlspp::Credential& cred, std::set<std::string> const& recognizedUserIDs) const;

	/**
	 * @brief Validate proposals message
	 * @param message authenticated content message
	 * @param targetState new state
	 * @param recognizedUserIDs recognised list of user IDs
	 * @return true if validated
	 */
	[[nodiscard]] bool validate_proposal_message(::mlspp::AuthenticatedContent const& message, ::mlspp::State const& targetState, std::set<std::string> const& recognizedUserIDs) const;

	/**
	 * @brief Verify that welcome state is valid
	 * @param state current state
	 * @param recognizedUserIDs list of recognised user IDs
	 * @return
	 */
	[[nodiscard]] bool verify_welcome_state(::mlspp::State const& state, std::set<std::string> const& recognizedUserIDs) const;

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
	std::list<queued_proposal> proposalQueue_;

	mls_failure_callback onMLSFailureCallback_{};

	/**
	 * @brief DPP Cluster, used for logging
	 */
	dpp::cluster& creator;
};

} // namespace dpp::dave::mls


