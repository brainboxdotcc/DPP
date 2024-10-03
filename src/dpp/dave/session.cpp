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

#include <hpke/random.h>
#include <hpke/signature.h>
#include <mls/crypto.h>
#include <mls/messages.h>
#include <mls/state.h>

#include "logger.h"
#include "mls_key_ratchet.h"
#include "user_credential.h"
#include "parameters.h"
#include "persisted_key_pair.h"
#include "util.h"

#include "openssl/evp.h"

#define TRACK_MLS_ERROR(reason)                      \
    if (onMLSFailureCallback_) {                     \
        onMLSFailureCallback_(__FUNCTION__, reason); \
    }

namespace dpp::dave::mls {

struct QueuedProposal {
    ::mlspp::ValidatedContent content;
    ::mlspp::bytes_ns::bytes ref;
};

Session::Session(KeyPairContextType context,
                 const std::string& authSessionId,
                 MLSFailureCallback callback) noexcept
  : signingKeyId_(authSessionId)
  , keyPairContext_(context)
  , onMLSFailureCallback_(std::move(callback))
{
    DISCORD_LOG(LS_INFO) << "Creating a new MLS session";
}

Session::~Session() noexcept = default;

void Session::Init(ProtocolVersion protocolVersion,
                   uint64_t groupId,
                   std::string const& selfUserId,
                   std::shared_ptr<::mlspp::SignaturePrivateKey>& transientKey) noexcept
{
    Reset();

    selfUserId_ = selfUserId;

    DISCORD_LOG(LS_INFO) << "Initializing MLS session with protocol version " << protocolVersion
                         << " and group ID " << groupId;
    protocolVersion_ = protocolVersion;
    groupId_ = std::move(BigEndianBytesFrom(groupId).as_vec());

    InitLeafNode(selfUserId, transientKey);

    CreatePendingGroup();
}

void Session::Reset() noexcept
{
    DISCORD_LOG(LS_INFO) << "Resetting MLS session";

    ClearPendingState();

    currentState_.reset();
    outboundCachedGroupState_.reset();

    protocolVersion_ = 0;
    groupId_.clear();
}

void Session::SetProtocolVersion(ProtocolVersion version) noexcept
{
    if (version != protocolVersion_) {
        // when we need to retain backwards compatibility
        // there may be some changes to the MLS objects required here
        // until then we can just update the stored version
        protocolVersion_ = version;
    }
}

std::vector<uint8_t> Session::GetLastEpochAuthenticator() const noexcept
{
    if (!currentState_) {
        DISCORD_LOG(LS_ERROR) << "Cannot get epoch authenticator without an established MLS group";
        return {};
    }

    return std::move(currentState_->epoch_authenticator().as_vec());
}

void Session::SetExternalSender(const std::vector<uint8_t>& marshalledExternalSender) noexcept
try {
    if (currentState_) {
        DISCORD_LOG(LS_ERROR) << "Cannot set external sender after joining/creating an MLS group";
        return;
    }

    DISCORD_LOG(LS_INFO) << "Unmarshalling MLS external sender";

    DISCORD_LOG(LS_INFO) << "Sender: " << ::mlspp::bytes_ns::bytes(marshalledExternalSender);

    externalSender_ = std::make_unique<::mlspp::ExternalSender>(
      ::mlspp::tls::get<::mlspp::ExternalSender>(marshalledExternalSender));

    if (!groupId_.empty()) {
        CreatePendingGroup();
    }
}
catch (const std::exception& e) {
    DISCORD_LOG(LS_ERROR) << "Failed to unmarshal external sender: " << e.what();
    TRACK_MLS_ERROR(e.what());
    return;
}

std::optional<std::vector<uint8_t>> Session::ProcessProposals(
  std::vector<uint8_t> proposals,
  std::set<std::string> const& recognizedUserIDs) noexcept
try {
    if (!pendingGroupState_ && !currentState_) {
        DISCORD_LOG(LS_ERROR)
          << "Cannot process proposals without any pending or established MLS group state";
        return std::nullopt;
    }

    if (!stateWithProposals_) {
        stateWithProposals_ = std::make_unique<::mlspp::State>(
          pendingGroupState_ ? *pendingGroupState_ : *currentState_);
    }

    DISCORD_LOG(LS_INFO) << "Processing MLS proposals message of " << proposals.size() << " bytes";

    DISCORD_LOG(LS_INFO) << "Proposals: " << ::mlspp::bytes_ns::bytes(proposals);

    ::mlspp::tls::istream inStream(proposals);

    bool isRevoke = false;
    inStream >> isRevoke;

    DISCORD_LOG(LS_INFO) << "Revoking: " << isRevoke;

    const auto suite = stateWithProposals_->cipher_suite();

    if (isRevoke) {
        std::vector<::mlspp::bytes_ns::bytes> refs;
        inStream >> refs;

        for (const auto& ref : refs) {
            bool found = false;
            for (auto it = proposalQueue_.begin(); it != proposalQueue_.end(); it++) {
                if (it->ref == ref) {
                    found = true;
                    proposalQueue_.erase(it);
                    break;
                }
            }

            if (!found) {
                DISCORD_LOG(LS_ERROR) << "Cannot revoke unrecognized proposal ref";
                TRACK_MLS_ERROR("Unrecognized proposal revocation");
                return std::nullopt;
            }
        }

        stateWithProposals_ = std::make_unique<::mlspp::State>(
          pendingGroupState_ ? *pendingGroupState_ : *currentState_);

        for (auto& prop : proposalQueue_) {
            // success will queue the proposal, failure will throw
            stateWithProposals_->handle(prop.content);
        }
    }
    else {
        std::vector<::mlspp::MLSMessage> messages;
        inStream >> messages;

        for (const auto& proposalMessage : messages) {
            auto validatedMessage = stateWithProposals_->unwrap(proposalMessage);

            if (!ValidateProposalMessage(validatedMessage.authenticated_content(),
                                         *stateWithProposals_,
                                         recognizedUserIDs)) {
                return std::nullopt;
            }

            // success will queue the proposal, failure will throw
            stateWithProposals_->handle(validatedMessage);

            auto ref = suite.ref(validatedMessage.authenticated_content());

            proposalQueue_.push_back({
              std::move(validatedMessage),
              std::move(ref),
            });
        }
    }

    // generate a commit
    auto commitSecret = ::mlspp::hpke::random_bytes(suite.secret_size());

    auto commitOpts = ::mlspp::CommitOpts{
      {},    // no extra proposals
      true,  // inline tree in welcome
      false, // do not force path
      {}     // default leaf node options
    };

    auto [commitMessage, welcomeMessage, newState] =
      stateWithProposals_->commit(commitSecret, commitOpts, {});

    DISCORD_LOG(LS_INFO)
      << "Prepared commit/welcome/next state for MLS group from received proposals";

    // combine the commit and welcome messages into a single buffer
    auto outStream = ::mlspp::tls::ostream();
    outStream << commitMessage;

    // keep a copy of the commit, we can check incoming pending group commit later for a match
    pendingGroupCommit_ = std::make_unique<::mlspp::MLSMessage>(std::move(commitMessage));

    // if there were any add proposals in this commit, then we also include the welcome message
    if (welcomeMessage.secrets.size() > 0) {
        outStream << welcomeMessage;
    }

    // cache the outbound state in case we're the winning sender
    outboundCachedGroupState_ = std::make_unique<::mlspp::State>(std::move(newState));

    DISCORD_LOG(LS_INFO) << "Output: " << ::mlspp::bytes_ns::bytes(outStream.bytes());

    return outStream.bytes();
}
catch (const std::exception& e) {
    DISCORD_LOG(LS_ERROR) << "Failed to parse MLS proposals: " << e.what();
    TRACK_MLS_ERROR(e.what());
    return std::nullopt;
}

bool Session::IsRecognizedUserID(const ::mlspp::Credential& cred,
                                 std::set<std::string> const& recognizedUserIDs) const
{
    std::string uid = UserCredentialToString(cred, protocolVersion_);
    if (uid.empty()) {
        DISCORD_LOG(LS_ERROR) << "Attempted to verify credential of unexpected type";
        return false;
    }

    if (recognizedUserIDs.find(uid) == recognizedUserIDs.end()) {
        DISCORD_LOG(LS_ERROR) << "Attempted to verify credential for unrecognized user ID: " << uid;
        return false;
    }

    return true;
}

bool Session::ValidateProposalMessage(::mlspp::AuthenticatedContent const& message,
                                      ::mlspp::State const& targetState,
                                      std::set<std::string> const& recognizedUserIDs) const
{
    if (message.wire_format != ::mlspp::WireFormat::mls_public_message) {
        DISCORD_LOG(LS_ERROR) << "MLS proposal message must be PublicMessage";
        TRACK_MLS_ERROR("Invalid proposal wire format");
        return false;
    }

    if (message.content.epoch != targetState.epoch()) {
        DISCORD_LOG(LS_ERROR) << "MLS proposal message must be for current epoch ("
                              << message.content.epoch << " != " << targetState.epoch() << ")";
        TRACK_MLS_ERROR("Proposal epoch mismatch");
        return false;
    }

    if (message.content.content_type() != ::mlspp::ContentType::proposal) {
        DISCORD_LOG(LS_ERROR) << "ProcessProposals called with non-proposal message";
        TRACK_MLS_ERROR("Unexpected message type");
        return false;
    }

    if (message.content.sender.sender_type() != ::mlspp::SenderType::external) {
        DISCORD_LOG(LS_ERROR) << "MLS proposal must be from external sender";
        TRACK_MLS_ERROR("Unexpected proposal sender type");
        return false;
    }

    const auto& proposal = ::mlspp::tls::var::get<::mlspp::Proposal>(message.content.content);
    switch (proposal.proposal_type()) {
    case ::mlspp::ProposalType::add: {
        const auto& credential =
          ::mlspp::tls::var::get<::mlspp::Add>(proposal.content).key_package.leaf_node.credential;
        if (!IsRecognizedUserID(credential, recognizedUserIDs)) {
            DISCORD_LOG(LS_ERROR) << "MLS add proposal must be for recognized user";
            TRACK_MLS_ERROR("Unexpected user ID in add proposal");
            return false;
        }
        break;
    }
    case ::mlspp::ProposalType::remove:
        // Remove proposals are always allowed (mlspp will validate that it's a recognized user)
        break;
    default:
        DISCORD_LOG(LS_ERROR) << "MLS proposal must be add or remove";
        TRACK_MLS_ERROR("Unexpected proposal type");
        return false;
    }

    return true;
}

bool Session::CanProcessCommit(const ::mlspp::MLSMessage& commit) noexcept
{
    if (!stateWithProposals_) {
        return false;
    }

    if (commit.group_id() != groupId_) {
        DISCORD_LOG(LS_ERROR) << "MLS commit message was for unexpected group";
        return false;
    }

    return true;
}

RosterVariant Session::ProcessCommit(std::vector<uint8_t> commit) noexcept
try {
    DISCORD_LOG(LS_INFO) << "Processing commit";
    DISCORD_LOG(LS_INFO) << "Commit: " << ::mlspp::bytes_ns::bytes(commit);

    auto commitMessage = ::mlspp::tls::get<::mlspp::MLSMessage>(commit);

    if (!CanProcessCommit(commitMessage)) {
        DISCORD_LOG(LS_ERROR) << "ProcessCommit called with unprocessable MLS commit";
        return ignored_t{};
    }

    // in case we're the sender of this commit
    // we need to pull the cached state from our outbound cache
    std::optional<::mlspp::State> optionalCachedState = std::nullopt;
    if (outboundCachedGroupState_) {
        optionalCachedState = *(outboundCachedGroupState_.get());
    }

    auto newState = stateWithProposals_->handle(commitMessage, optionalCachedState);

    if (!newState) {
        DISCORD_LOG(LS_ERROR) << "MLS commit handling did not produce a new state";
        return failed_t{};
    }

    DISCORD_LOG(LS_INFO) << "Successfully processed MLS commit, updating state; our leaf index is "
                         << newState->index().val << "; current epoch is " << newState->epoch();

    RosterMap ret = ReplaceState(std::make_unique<::mlspp::State>(std::move(*newState)));

    // reset the outbound cached group since we handled the commit for this epoch
    outboundCachedGroupState_.reset();

    ClearPendingState();

    return ret;
}
catch (const std::exception& e) {
    DISCORD_LOG(LS_ERROR) << "Failed to process MLS commit: " << e.what();
    TRACK_MLS_ERROR(e.what());
    return failed_t{};
}

std::optional<RosterMap> Session::ProcessWelcome(
  std::vector<uint8_t> welcome,
  std::set<std::string> const& recognizedUserIDs) noexcept
try {
    if (!HasCryptographicStateForWelcome()) {
        DISCORD_LOG(LS_ERROR) << "Missing local crypto state necessary to process MLS welcome";
        return std::nullopt;
    }

    if (!externalSender_) {
        DISCORD_LOG(LS_ERROR) << "Cannot process MLS welcome without an external sender";
        return std::nullopt;
    }

    if (currentState_) {
        DISCORD_LOG(LS_ERROR) << "Cannot process MLS welcome after joining/creating an MLS group";
        return std::nullopt;
    }

    DISCORD_LOG(LS_INFO) << "Processing welcome: " << ::mlspp::bytes_ns::bytes(welcome);

    // unmarshal the incoming welcome
    auto unmarshalledWelcome = ::mlspp::tls::get<::mlspp::Welcome>(welcome);

    // construct the state from the unmarshalled welcome
    auto newState = std::make_unique<::mlspp::State>(
      *joinInitPrivateKey_,
      *selfHPKEPrivateKey_,
      *selfSigPrivateKey_,
      *joinKeyPackage_,
      unmarshalledWelcome,
      std::nullopt,
      std::map<::mlspp::bytes_ns::bytes, ::mlspp::bytes_ns::bytes>());

    // perform application-level verification of the new state
    if (!VerifyWelcomeState(*newState, recognizedUserIDs)) {
        DISCORD_LOG(LS_ERROR) << "Group received in MLS welcome is not valid";

        return std::nullopt;
    }

    DISCORD_LOG(LS_INFO) << "Successfully welcomed to MLS Group, our leaf index is "
                         << newState->index().val << "; current epoch is " << newState->epoch();

    // make the verified state our new (and only) state
    RosterMap ret = ReplaceState(std::move(newState));

    // clear out any pending state for creating/joining a group
    ClearPendingState();

    return ret;
}
catch (const std::exception& e) {
    DISCORD_LOG(LS_ERROR) << "Failed to create group state from MLS welcome: " << e.what();
    TRACK_MLS_ERROR(e.what());
    return std::nullopt;
}

RosterMap Session::ReplaceState(std::unique_ptr<::mlspp::State>&& state)
{
    RosterMap newRoster;
    for (const ::mlspp::LeafNode& node : state->roster()) {
        if (node.credential.type() != ::mlspp::CredentialType::basic) {
            continue;
        }

        const auto& cred = node.credential.template get<::mlspp::BasicCredential>();

        newRoster[FromBigEndianBytes(cred.identity)] = node.signature_key.data.as_vec();
    }

    RosterMap changeMap;

    std::set_difference(newRoster.begin(),
                        newRoster.end(),
                        roster_.begin(),
                        roster_.end(),
                        std::inserter(changeMap, changeMap.end()));

    struct MissingItemWrapper {
        RosterMap& changeMap_;

        using iterator = RosterMap::iterator;
        using const_iterator = RosterMap::const_iterator;
        using value_type = RosterMap::value_type;

        iterator insert(const_iterator it, const value_type& value)
        {
            return changeMap_.try_emplace(it, value.first, std::vector<uint8_t>{});
        }

        iterator begin() { return changeMap_.begin(); }

        iterator end() { return changeMap_.end(); }
    };

    MissingItemWrapper wrapper{changeMap};

    std::set_difference(roster_.begin(),
                        roster_.end(),
                        newRoster.begin(),
                        newRoster.end(),
                        std::inserter(wrapper, wrapper.end()));

    roster_ = std::move(newRoster);
    currentState_ = std::move(state);

    return changeMap;
}

bool Session::HasCryptographicStateForWelcome() const noexcept
{
    return joinKeyPackage_ && joinInitPrivateKey_ && selfSigPrivateKey_ && selfHPKEPrivateKey_;
}

bool Session::VerifyWelcomeState(::mlspp::State const& state,
                                 std::set<std::string> const& recognizedUserIDs) const
{
    if (!externalSender_) {
        DISCORD_LOG(LS_ERROR) << "Cannot verify MLS welcome without an external sender";
        TRACK_MLS_ERROR("Missing external sender when processing Welcome");
        return false;
    }

    auto ext = state.extensions().template find<mlspp::ExternalSendersExtension>();
    if (!ext) {
        DISCORD_LOG(LS_ERROR) << "MLS welcome missing external senders extension";
        TRACK_MLS_ERROR("Welcome message missing external sender extension");
        return false;
    }

    if (ext->senders.size() != 1) {
        DISCORD_LOG(LS_ERROR) << "MLS welcome lists unexpected number of external senders: "
                              << ext->senders.size();
        TRACK_MLS_ERROR("Welcome message lists unexpected external sender count");
        return false;
    }

    if (ext->senders.front() != *externalSender_) {
        DISCORD_LOG(LS_ERROR) << "MLS welcome lists unexpected external sender";
        TRACK_MLS_ERROR("Welcome message lists unexpected external sender");
        return false;
    }

    // TODO: Until we leverage revocation in the protocol
    // if we re-enable this change we will refuse welcome messages
    // because someone was previously supposed to be added but disconnected
    // before all in-flight proposals were handled.

    for (const auto& leaf : state.roster()) {
        if (!IsRecognizedUserID(leaf.credential, recognizedUserIDs)) {
            DISCORD_LOG(LS_ERROR) << "MLS welcome lists unrecognized user ID";
            // TRACK_MLS_ERROR("Welcome message lists unrecognized user ID");
            // return false;
        }
    }

    return true;
}

void Session::InitLeafNode(std::string const& selfUserId,
                           std::shared_ptr<::mlspp::SignaturePrivateKey>& transientKey) noexcept
try {
    auto ciphersuite = CiphersuiteForProtocolVersion(protocolVersion_);

    if (!transientKey) {
	    std::cout << "2\n";
        if (!signingKeyId_.empty()) {
		std::cout << "3\n";
            transientKey = GetPersistedKeyPair(keyPairContext_, signingKeyId_, protocolVersion_);
            if (!transientKey) {
                DISCORD_LOG(LS_ERROR) << "Did not receive MLS signature private key from "
                                         "GetPersistedKeyPair; aborting";
                return;
            }
        }
        else {
		std::cout << "1\n";
            transientKey = std::make_shared<::mlspp::SignaturePrivateKey>(
              ::mlspp::SignaturePrivateKey::generate(ciphersuite));
        }
    }

    selfSigPrivateKey_ = transientKey;

    auto selfCredential = CreateUserCredential(selfUserId, protocolVersion_);

    selfHPKEPrivateKey_ =
      std::make_unique<::mlspp::HPKEPrivateKey>(::mlspp::HPKEPrivateKey::generate(ciphersuite));

    selfLeafNode_ =
      std::make_unique<::mlspp::LeafNode>(ciphersuite,
                                          selfHPKEPrivateKey_->public_key,
                                          selfSigPrivateKey_->public_key,
                                          std::move(selfCredential),
                                          LeafNodeCapabilitiesForProtocolVersion(protocolVersion_),
                                          ::mlspp::Lifetime::create_default(),
                                          LeafNodeExtensionsForProtocolVersion(protocolVersion_),
                                          *selfSigPrivateKey_);

    DISCORD_LOG(LS_INFO) << "Created MLS leaf node";
}
catch (const std::exception& e) {
    DISCORD_LOG(LS_INFO) << "Failed to initialize MLS leaf node: " << e.what();
    TRACK_MLS_ERROR(e.what());
}

void Session::ResetJoinKeyPackage() noexcept
try {
    if (!selfLeafNode_) {
        DISCORD_LOG(LS_ERROR) << "Cannot initialize join key package without a leaf node";
        return;
    }

    auto ciphersuite = CiphersuiteForProtocolVersion(protocolVersion_);

    joinInitPrivateKey_ =
      std::make_unique<::mlspp::HPKEPrivateKey>(::mlspp::HPKEPrivateKey::generate(ciphersuite));

    joinKeyPackage_ =
      std::make_unique<::mlspp::KeyPackage>(ciphersuite,
                                            joinInitPrivateKey_->public_key,
                                            *selfLeafNode_,
                                            LeafNodeExtensionsForProtocolVersion(protocolVersion_),
                                            *selfSigPrivateKey_);

    DISCORD_LOG(LS_INFO) << "Generated key package: "
                         << ::mlspp::bytes_ns::bytes(::mlspp::tls::marshal(*joinKeyPackage_));
}
catch (const std::exception& e) {
    DISCORD_LOG(LS_ERROR) << "Failed to initialize join key package: " << e.what();
    TRACK_MLS_ERROR(e.what());
}

void Session::CreatePendingGroup() noexcept
try {
    if (groupId_.empty()) {
        DISCORD_LOG(LS_ERROR) << "Cannot create MLS group without a group ID";
        return;
    }

    if (!externalSender_) {
        DISCORD_LOG(LS_ERROR) << "Cannot create MLS group without ExternalSender";
        return;
    }

    if (!selfLeafNode_) {
        DISCORD_LOG(LS_ERROR) << "Cannot create MLS group without self leaf node";
        return;
    }

    DISCORD_LOG(LS_INFO) << "Creating a pending MLS group";

    auto ciphersuite = CiphersuiteForProtocolVersion(protocolVersion_);

    pendingGroupState_ = std::make_unique<::mlspp::State>(
      groupId_,
      ciphersuite,
      *selfHPKEPrivateKey_,
      *selfSigPrivateKey_,
      *selfLeafNode_,
      GroupExtensionsForProtocolVersion(protocolVersion_, *externalSender_));

    DISCORD_LOG(LS_INFO) << "Created a pending MLS group";
}
catch (const std::exception& e) {
    DISCORD_LOG(LS_ERROR) << "Failed to create MLS group: " << e.what();
    TRACK_MLS_ERROR(e.what());
    return;
}

std::vector<uint8_t> Session::GetMarshalledKeyPackage() noexcept
try {
    // key packages are not meant to be re-used
    // so every time the client asks for a key package we create a new one
    ResetJoinKeyPackage();

    if (!joinKeyPackage_) {
        DISCORD_LOG(LS_ERROR) << "Cannot marshal an uninitialized key package";
        return {};
    }

    return ::mlspp::tls::marshal(*joinKeyPackage_);
}
catch (const std::exception& e) {
    DISCORD_LOG(LS_ERROR) << "Failed to marshal join key package: " << e.what();
    TRACK_MLS_ERROR(e.what());
    return {};
}

std::unique_ptr<IKeyRatchet> Session::GetKeyRatchet(std::string const& userId) const noexcept
{
    if (!currentState_) {
        DISCORD_LOG(LS_ERROR) << "Cannot get key ratchet without an established MLS group";
        return nullptr;
    }

    // change the string user ID to a little endian 64 bit user ID
    auto u64userId = strtoull(userId.c_str(), nullptr, 10);
    auto userIdBytes = ::mlspp::bytes_ns::bytes(sizeof(u64userId));
    memcpy(userIdBytes.data(), &u64userId, sizeof(u64userId));

    // generate the base secret for the hash ratchet
    auto baseSecret =
      currentState_->do_export(Session::USER_MEDIA_KEY_BASE_LABEL, userIdBytes, kAesGcm128KeyBytes);

    // this assumes the MLS ciphersuite produces a kAesGcm128KeyBytes sized key
    // would need to be updated to a different ciphersuite if there's a future mismatch
    return std::make_unique<MlsKeyRatchet>(currentState_->cipher_suite(), std::move(baseSecret));
}

void Session::GetPairwiseFingerprint(uint16_t version,
                                     std::string const& userId,
                                     PairwiseFingerprintCallback callback) const noexcept
try {
    if (!currentState_ || !selfSigPrivateKey_) {
        throw std::invalid_argument("No established MLS group");
    }

    uint64_t u64RemoteUserId = strtoull(userId.c_str(), nullptr, 10);
    uint64_t u64SelfUserId = strtoull(selfUserId_.c_str(), nullptr, 10);

    auto it = roster_.find(u64RemoteUserId);
    if (it == roster_.end()) {
        throw std::invalid_argument("Unknown user ID: " + userId);
    }

    ::mlspp::tls::ostream toHash1;
    ::mlspp::tls::ostream toHash2;

    toHash1 << version;
    toHash1.write_raw(it->second);
    toHash1 << u64RemoteUserId;

    toHash2 << version;
    toHash2.write_raw(selfSigPrivateKey_->public_key.data);
    toHash2 << u64SelfUserId;

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

        int ret = EVP_PBE_scrypt((const char*)data.data(),
                                 data.size(),
                                 salt,
                                 sizeof(salt),
                                 N,
                                 r,
                                 p,
                                 max_mem,
                                 out.data(),
                                 out.size());

        if (ret == 1) {
            callback(out);
        }
        else {
            callback({});
        }
    }).detach();
}
catch (const std::exception& e) {
    DISCORD_LOG(LS_ERROR) << "Failed to generate pairwise fingerprint: " << e.what();
    callback({});
}

void Session::ClearPendingState()
{
    pendingGroupState_.reset();
    pendingGroupCommit_.reset();

    joinInitPrivateKey_.reset();
    joinKeyPackage_.reset();

    selfHPKEPrivateKey_.reset();

    selfLeafNode_.reset();

    stateWithProposals_.reset();
    proposalQueue_.clear();
}

} // namespace dpp::dave::mls


