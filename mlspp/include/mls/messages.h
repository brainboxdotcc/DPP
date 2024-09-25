#pragma once

#include "mls/common.h"
#include "mls/core_types.h"
#include "mls/credential.h"
#include "mls/crypto.h"
#include "mls/treekem.h"
#include <optional>
#include <tls/tls_syntax.h>

namespace mlspp {

struct ExternalPubExtension
{
  HPKEPublicKey external_pub;

  static const uint16_t type;
  TLS_SERIALIZABLE(external_pub)
};

struct RatchetTreeExtension
{
  TreeKEMPublicKey tree;

  static const uint16_t type;
  TLS_SERIALIZABLE(tree)
};

struct ExternalSender
{
  SignaturePublicKey signature_key;
  Credential credential;

  TLS_SERIALIZABLE(signature_key, credential);
};

struct ExternalSendersExtension
{
  std::vector<ExternalSender> senders;

  static const uint16_t type;
  TLS_SERIALIZABLE(senders);
};

struct SFrameParameters
{
  uint16_t cipher_suite;
  uint8_t epoch_bits;

  static const uint16_t type;
  TLS_SERIALIZABLE(cipher_suite, epoch_bits)
};

struct SFrameCapabilities
{
  std::vector<uint16_t> cipher_suites;

  bool compatible(const SFrameParameters& params) const;

  static const uint16_t type;
  TLS_SERIALIZABLE(cipher_suites)
};

///
/// PSKs
///
enum struct PSKType : uint8_t
{
  reserved = 0,
  external = 1,
  resumption = 2,
};

struct ExternalPSK
{
  bytes psk_id;
  TLS_SERIALIZABLE(psk_id)
};

enum struct ResumptionPSKUsage : uint8_t
{
  reserved = 0,
  application = 1,
  reinit = 2,
  branch = 3,
};

struct ResumptionPSK
{
  ResumptionPSKUsage usage;
  bytes psk_group_id;
  epoch_t psk_epoch;
  TLS_SERIALIZABLE(usage, psk_group_id, psk_epoch)
};

struct PreSharedKeyID
{
  var::variant<ExternalPSK, ResumptionPSK> content;
  bytes psk_nonce;
  TLS_SERIALIZABLE(content, psk_nonce)
  TLS_TRAITS(tls::variant<PSKType>, tls::pass)
};

struct PreSharedKeys
{
  std::vector<PreSharedKeyID> psks;
  TLS_SERIALIZABLE(psks)
};

struct PSKWithSecret
{
  PreSharedKeyID id;
  bytes secret;
};

// struct {
//     ProtocolVersion version = mls10;
//     CipherSuite cipher_suite;
//     opaque group_id<V>;
//     uint64 epoch;
//     opaque tree_hash<V>;
//     opaque confirmed_transcript_hash<V>;
//     Extension extensions<V>;
// } GroupContext;
struct GroupContext
{
  ProtocolVersion version{ ProtocolVersion::mls10 };
  CipherSuite cipher_suite;
  bytes group_id;
  epoch_t epoch;
  bytes tree_hash;
  bytes confirmed_transcript_hash;
  ExtensionList extensions;

  GroupContext() = default;
  GroupContext(CipherSuite cipher_suite_in,
               bytes group_id_in,
               epoch_t epoch_in,
               bytes tree_hash_in,
               bytes confirmed_transcript_hash_in,
               ExtensionList extensions_in);

  TLS_SERIALIZABLE(version,
                   cipher_suite,
                   group_id,
                   epoch,
                   tree_hash,
                   confirmed_transcript_hash,
                   extensions)
};

// struct {
//     GroupContext group_context;
//     Extension extensions<V>;
//     MAC confirmation_tag;
//     uint32 signer;
//     // SignWithLabel(., "GroupInfoTBS", GroupInfoTBS)
//     opaque signature<V>;
// } GroupInfo;
struct GroupInfo
{
  GroupContext group_context;
  ExtensionList extensions;
  bytes confirmation_tag;
  LeafIndex signer;
  bytes signature;

  GroupInfo() = default;
  GroupInfo(GroupContext group_context_in,
            ExtensionList extensions_in,
            bytes confirmation_tag_in);

  bytes to_be_signed() const;
  void sign(const TreeKEMPublicKey& tree,
            LeafIndex signer_index,
            const SignaturePrivateKey& priv);
  bool verify(const TreeKEMPublicKey& tree) const;

  // These methods exist only to simplify unit testing
  void sign(LeafIndex signer_index, const SignaturePrivateKey& priv);
  bool verify(const SignaturePublicKey& pub) const;

  TLS_SERIALIZABLE(group_context,
                   extensions,
                   confirmation_tag,
                   signer,
                   signature)
};

// struct {
//   opaque joiner_secret<1..255>;
//   optional<PathSecret> path_secret;
//   PreSharedKeys psks;
// } GroupSecrets;
struct GroupSecrets
{
  struct PathSecret
  {
    bytes secret;

    TLS_SERIALIZABLE(secret)
  };

  bytes joiner_secret;
  std::optional<PathSecret> path_secret;
  PreSharedKeys psks;

  TLS_SERIALIZABLE(joiner_secret, path_secret, psks)
};

// struct {
//   opaque key_package_hash<1..255>;
//   HPKECiphertext encrypted_group_secrets;
// } EncryptedGroupSecrets;
struct EncryptedGroupSecrets
{
  KeyPackageRef new_member;
  HPKECiphertext encrypted_group_secrets;

  TLS_SERIALIZABLE(new_member, encrypted_group_secrets)
};

// struct {
//   ProtocolVersion version = mls10;
//   CipherSuite cipher_suite;
//   EncryptedGroupSecrets group_secretss<1..2^32-1>;
//   opaque encrypted_group_info<1..2^32-1>;
// } Welcome;
struct Welcome
{
  CipherSuite cipher_suite;
  std::vector<EncryptedGroupSecrets> secrets;
  bytes encrypted_group_info;

  Welcome();
  Welcome(CipherSuite suite,
          const bytes& joiner_secret,
          const std::vector<PSKWithSecret>& psks,
          const GroupInfo& group_info);

  void encrypt(const KeyPackage& kp, const std::optional<bytes>& path_secret);
  std::optional<int> find(const KeyPackage& kp) const;
  GroupSecrets decrypt_secrets(int kp_index,
                               const HPKEPrivateKey& init_priv) const;
  GroupInfo decrypt(const bytes& joiner_secret,
                    const std::vector<PSKWithSecret>& psks) const;

  TLS_SERIALIZABLE(cipher_suite, secrets, encrypted_group_info)

private:
  bytes _joiner_secret;
  PreSharedKeys _psks;
  static KeyAndNonce group_info_key_nonce(
    CipherSuite suite,
    const bytes& joiner_secret,
    const std::vector<PSKWithSecret>& psks);
};

///
/// Proposals & Commit
///

// Add
struct Add
{
  KeyPackage key_package;
  TLS_SERIALIZABLE(key_package)
};

// Update
struct Update
{
  LeafNode leaf_node;
  TLS_SERIALIZABLE(leaf_node)
};

// Remove
struct Remove
{
  LeafIndex removed;
  TLS_SERIALIZABLE(removed)
};

// PreSharedKey
struct PreSharedKey
{
  PreSharedKeyID psk;
  TLS_SERIALIZABLE(psk)
};

// ReInit
struct ReInit
{
  bytes group_id;
  ProtocolVersion version;
  CipherSuite cipher_suite;
  ExtensionList extensions;

  TLS_SERIALIZABLE(group_id, version, cipher_suite, extensions)
};

// ExternalInit
struct ExternalInit
{
  bytes kem_output;
  TLS_SERIALIZABLE(kem_output)
};

// GroupContextExtensions
struct GroupContextExtensions
{
  ExtensionList group_context_extensions;
  TLS_SERIALIZABLE(group_context_extensions)
};

struct ProposalType;

struct Proposal
{
  using Type = uint16_t;

  var::variant<Add,
               Update,
               Remove,
               PreSharedKey,
               ReInit,
               ExternalInit,
               GroupContextExtensions>
    content;

  Type proposal_type() const;

  TLS_SERIALIZABLE(content)
  TLS_TRAITS(tls::variant<ProposalType>)
};

struct ProposalType
{
  static constexpr Proposal::Type invalid = 0;
  static constexpr Proposal::Type add = 1;
  static constexpr Proposal::Type update = 2;
  static constexpr Proposal::Type remove = 3;
  static constexpr Proposal::Type psk = 4;
  static constexpr Proposal::Type reinit = 5;
  static constexpr Proposal::Type external_init = 6;
  static constexpr Proposal::Type group_context_extensions = 7;

  constexpr ProposalType()
    : val(invalid)
  {
  }

  constexpr ProposalType(Proposal::Type pt)
    : val(pt)
  {
  }

  Proposal::Type val;
  TLS_SERIALIZABLE(val)
};

enum struct ProposalOrRefType : uint8_t
{
  reserved = 0,
  value = 1,
  reference = 2,
};

struct ProposalOrRef
{
  var::variant<Proposal, ProposalRef> content;

  TLS_SERIALIZABLE(content)
  TLS_TRAITS(tls::variant<ProposalOrRefType>)
};

// struct {
//     ProposalOrRef proposals<0..2^32-1>;
//     optional<UpdatePath> path;
// } Commit;
struct Commit
{
  std::vector<ProposalOrRef> proposals;
  std::optional<UpdatePath> path;

  // Validate that the commit is acceptable as an external commit, and if so,
  // produce the public key from the ExternalInit proposal
  std::optional<bytes> valid_external() const;

  TLS_SERIALIZABLE(proposals, path)
};

// struct {
//     opaque group_id<0..255>;
//     uint32 epoch;
//     uint32 sender;
//     ContentType content_type;
//
//     select (PublicMessage.content_type) {
//         case handshake:
//             GroupOperation operation;
//             opaque confirmation<0..255>;
//
//         case application:
//             opaque application_data<0..2^32-1>;
//     }
//
//     opaque signature<0..2^16-1>;
// } PublicMessage;
struct ApplicationData
{
  bytes data;
  TLS_SERIALIZABLE(data)
};

struct GroupContext;

enum struct WireFormat : uint16_t
{
  reserved = 0,
  mls_public_message = 1,
  mls_private_message = 2,
  mls_welcome = 3,
  mls_group_info = 4,
  mls_key_package = 5,
};

enum struct ContentType : uint8_t
{
  invalid = 0,
  application = 1,
  proposal = 2,
  commit = 3,
};

enum struct SenderType : uint8_t
{
  invalid = 0,
  member = 1,
  external = 2,
  new_member_proposal = 3,
  new_member_commit = 4,
};

struct MemberSender
{
  LeafIndex sender;
  TLS_SERIALIZABLE(sender);
};

struct ExternalSenderIndex
{
  uint32_t sender_index;
  TLS_SERIALIZABLE(sender_index)
};

struct NewMemberProposalSender
{
  TLS_SERIALIZABLE()
};

struct NewMemberCommitSender
{
  TLS_SERIALIZABLE()
};

struct Sender
{
  var::variant<MemberSender,
               ExternalSenderIndex,
               NewMemberProposalSender,
               NewMemberCommitSender>
    sender;

  SenderType sender_type() const;

  TLS_SERIALIZABLE(sender)
  TLS_TRAITS(tls::variant<SenderType>)
};

///
/// MLSMessage and friends
///
struct GroupKeySource;

struct GroupContent
{
  using RawContent = var::variant<ApplicationData, Proposal, Commit>;

  bytes group_id;
  epoch_t epoch;
  Sender sender;
  bytes authenticated_data;
  RawContent content;

  GroupContent() = default;
  GroupContent(bytes group_id_in,
               epoch_t epoch_in,
               Sender sender_in,
               bytes authenticated_data_in,
               RawContent content_in);
  GroupContent(bytes group_id_in,
               epoch_t epoch_in,
               Sender sender_in,
               bytes authenticated_data_in,
               ContentType content_type);

  ContentType content_type() const;

  TLS_SERIALIZABLE(group_id, epoch, sender, authenticated_data, content)
  TLS_TRAITS(tls::pass,
             tls::pass,
             tls::pass,
             tls::pass,
             tls::variant<ContentType>)
};

struct GroupContentAuthData
{
  ContentType content_type = ContentType::invalid;
  bytes signature;
  std::optional<bytes> confirmation_tag;

  friend tls::ostream& operator<<(tls::ostream& str,
                                  const GroupContentAuthData& obj);
  friend tls::istream& operator>>(tls::istream& str, GroupContentAuthData& obj);
  friend bool operator==(const GroupContentAuthData& lhs,
                         const GroupContentAuthData& rhs);
};

struct AuthenticatedContent
{
  WireFormat wire_format;
  GroupContent content;
  GroupContentAuthData auth;

  AuthenticatedContent() = default;

  static AuthenticatedContent sign(WireFormat wire_format,
                                   GroupContent content,
                                   CipherSuite suite,
                                   const SignaturePrivateKey& sig_priv,
                                   const std::optional<GroupContext>& context);
  bool verify(CipherSuite suite,
              const SignaturePublicKey& sig_pub,
              const std::optional<GroupContext>& context) const;

  bytes confirmed_transcript_hash_input() const;
  bytes interim_transcript_hash_input() const;

  void set_confirmation_tag(const bytes& confirmation_tag);
  bool check_confirmation_tag(const bytes& confirmation_tag) const;

  friend tls::ostream& operator<<(tls::ostream& str,
                                  const AuthenticatedContent& obj);
  friend tls::istream& operator>>(tls::istream& str, AuthenticatedContent& obj);
  friend bool operator==(const AuthenticatedContent& lhs,
                         const AuthenticatedContent& rhs);

private:
  AuthenticatedContent(WireFormat wire_format_in, GroupContent content_in);
  AuthenticatedContent(WireFormat wire_format_in,
                       GroupContent content_in,
                       GroupContentAuthData auth_in);

  bytes to_be_signed(const std::optional<GroupContext>& context) const;

  friend struct PublicMessage;
  friend struct PrivateMessage;
};

struct ValidatedContent
{
  const AuthenticatedContent& authenticated_content() const;

  friend bool operator==(const ValidatedContent& lhs,
                         const ValidatedContent& rhs);

private:
  AuthenticatedContent content_auth;

  ValidatedContent(AuthenticatedContent content_auth_in);

  friend struct PublicMessage;
  friend struct PrivateMessage;
  friend class State;
};

struct PublicMessage
{
  PublicMessage() = default;

  bytes get_group_id() const { return content.group_id; }
  epoch_t get_epoch() const { return content.epoch; }

  static PublicMessage protect(AuthenticatedContent content_auth,
                               CipherSuite suite,
                               const std::optional<bytes>& membership_key,
                               const std::optional<GroupContext>& context);
  std::optional<ValidatedContent> unprotect(
    CipherSuite suite,
    const std::optional<bytes>& membership_key,
    const std::optional<GroupContext>& context) const;

  bool contains(const AuthenticatedContent& content_auth) const;

  // TODO(RLB) Make this private and expose only to tests
  AuthenticatedContent authenticated_content() const;

  friend tls::ostream& operator<<(tls::ostream& str, const PublicMessage& obj);
  friend tls::istream& operator>>(tls::istream& str, PublicMessage& obj);
  friend bool operator==(const PublicMessage& lhs, const PublicMessage& rhs);
  friend bool operator!=(const PublicMessage& lhs, const PublicMessage& rhs);

private:
  GroupContent content;
  GroupContentAuthData auth;
  std::optional<bytes> membership_tag;

  PublicMessage(AuthenticatedContent content_auth);

  bytes membership_mac(CipherSuite suite,
                       const bytes& membership_key,
                       const std::optional<GroupContext>& context) const;
};

struct PrivateMessage
{
  PrivateMessage() = default;

  bytes get_group_id() const { return group_id; }
  epoch_t get_epoch() const { return epoch; }

  static PrivateMessage protect(AuthenticatedContent content_auth,
                                CipherSuite suite,
                                GroupKeySource& keys,
                                const bytes& sender_data_secret,
                                size_t padding_size);
  std::optional<ValidatedContent> unprotect(
    CipherSuite suite,
    GroupKeySource& keys,
    const bytes& sender_data_secret) const;

  TLS_SERIALIZABLE(group_id,
                   epoch,
                   content_type,
                   authenticated_data,
                   encrypted_sender_data,
                   ciphertext)

private:
  bytes group_id;
  epoch_t epoch;
  ContentType content_type;
  bytes authenticated_data;
  bytes encrypted_sender_data;
  bytes ciphertext;

  PrivateMessage(GroupContent content,
                 bytes encrypted_sender_data_in,
                 bytes ciphertext_in);
};

struct MLSMessage
{
  ProtocolVersion version = ProtocolVersion::mls10;
  var::variant<PublicMessage, PrivateMessage, Welcome, GroupInfo, KeyPackage>
    message;

  bytes group_id() const;
  epoch_t epoch() const;
  WireFormat wire_format() const;

  MLSMessage() = default;
  MLSMessage(PublicMessage public_message);
  MLSMessage(PrivateMessage private_message);
  MLSMessage(Welcome welcome);
  MLSMessage(GroupInfo group_info);
  MLSMessage(KeyPackage key_package);

  TLS_SERIALIZABLE(version, message)
  TLS_TRAITS(tls::pass, tls::variant<WireFormat>)
};

MLSMessage
external_proposal(CipherSuite suite,
                  const bytes& group_id,
                  epoch_t epoch,
                  const Proposal& proposal,
                  uint32_t signer_index,
                  const SignaturePrivateKey& sig_priv);

} // namespace mlspp

namespace mlspp::tls {

TLS_VARIANT_MAP(mlspp::PSKType, mlspp::ExternalPSK, external)
TLS_VARIANT_MAP(mlspp::PSKType,
                mlspp::ResumptionPSK,
                resumption)

TLS_VARIANT_MAP(mlspp::ProposalOrRefType,
                mlspp::Proposal,
                value)
TLS_VARIANT_MAP(mlspp::ProposalOrRefType,
                mlspp::ProposalRef,
                reference)

TLS_VARIANT_MAP(mlspp::ProposalType, mlspp::Add, add)
TLS_VARIANT_MAP(mlspp::ProposalType, mlspp::Update, update)
TLS_VARIANT_MAP(mlspp::ProposalType, mlspp::Remove, remove)
TLS_VARIANT_MAP(mlspp::ProposalType, mlspp::PreSharedKey, psk)
TLS_VARIANT_MAP(mlspp::ProposalType, mlspp::ReInit, reinit)
TLS_VARIANT_MAP(mlspp::ProposalType,
                mlspp::ExternalInit,
                external_init)
TLS_VARIANT_MAP(mlspp::ProposalType,
                mlspp::GroupContextExtensions,
                group_context_extensions)

TLS_VARIANT_MAP(mlspp::ContentType,
                mlspp::ApplicationData,
                application)
TLS_VARIANT_MAP(mlspp::ContentType, mlspp::Proposal, proposal)
TLS_VARIANT_MAP(mlspp::ContentType, mlspp::Commit, commit)

TLS_VARIANT_MAP(mlspp::SenderType, mlspp::MemberSender, member)
TLS_VARIANT_MAP(mlspp::SenderType,
                mlspp::ExternalSenderIndex,
                external)
TLS_VARIANT_MAP(mlspp::SenderType,
                mlspp::NewMemberProposalSender,
                new_member_proposal)
TLS_VARIANT_MAP(mlspp::SenderType,
                mlspp::NewMemberCommitSender,
                new_member_commit)

TLS_VARIANT_MAP(mlspp::WireFormat,
                mlspp::PublicMessage,
                mls_public_message)
TLS_VARIANT_MAP(mlspp::WireFormat,
                mlspp::PrivateMessage,
                mls_private_message)
TLS_VARIANT_MAP(mlspp::WireFormat, mlspp::Welcome, mls_welcome)
TLS_VARIANT_MAP(mlspp::WireFormat,
                mlspp::GroupInfo,
                mls_group_info)
TLS_VARIANT_MAP(mlspp::WireFormat,
                mlspp::KeyPackage,
                mls_key_package)

} // namespace mlspp::tls
