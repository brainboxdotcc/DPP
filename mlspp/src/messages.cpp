#include <mls/key_schedule.h>
#include <mls/messages.h>
#include <mls/state.h>
#include <mls/treekem.h>

#include "grease.h"

namespace mlspp {

// Extensions

const Extension::Type ExternalPubExtension::type = ExtensionType::external_pub;
const Extension::Type RatchetTreeExtension::type = ExtensionType::ratchet_tree;
const Extension::Type ExternalSendersExtension::type =
  ExtensionType::external_senders;
const Extension::Type SFrameParameters::type = ExtensionType::sframe_parameters;
const Extension::Type SFrameCapabilities::type =
  ExtensionType::sframe_parameters;

bool
SFrameCapabilities::compatible(const SFrameParameters& params) const
{
  return stdx::contains(cipher_suites, params.cipher_suite);
}

// GroupContext

GroupContext::GroupContext(CipherSuite cipher_suite_in,
                           bytes group_id_in,
                           epoch_t epoch_in,
                           bytes tree_hash_in,
                           bytes confirmed_transcript_hash_in,
                           ExtensionList extensions_in)
  : cipher_suite(cipher_suite_in)
  , group_id(std::move(group_id_in))
  , epoch(epoch_in)
  , tree_hash(std::move(tree_hash_in))
  , confirmed_transcript_hash(std::move(confirmed_transcript_hash_in))
  , extensions(std::move(extensions_in))
{
}

// GroupInfo

GroupInfo::GroupInfo(GroupContext group_context_in,
                     ExtensionList extensions_in,
                     bytes confirmation_tag_in)
  : group_context(std::move(group_context_in))
  , extensions(std::move(extensions_in))
  , confirmation_tag(std::move(confirmation_tag_in))
  , signer(0)
{
  grease(extensions);
}

struct GroupInfoTBS
{
  GroupContext group_context;
  ExtensionList extensions;
  bytes confirmation_tag;
  LeafIndex signer;

  TLS_SERIALIZABLE(group_context, extensions, confirmation_tag, signer)
};

bytes
GroupInfo::to_be_signed() const
{
  return tls::marshal(
    GroupInfoTBS{ group_context, extensions, confirmation_tag, signer });
}

void
GroupInfo::sign(const TreeKEMPublicKey& tree,
                LeafIndex signer_index,
                const SignaturePrivateKey& priv)
{
  auto maybe_leaf = tree.leaf_node(signer_index);
  if (!maybe_leaf) {
    throw InvalidParameterError("Cannot sign from a blank leaf");
  }

  if (priv.public_key != opt::get(maybe_leaf).signature_key) {
    throw InvalidParameterError("Bad key for index");
  }

  signer = signer_index;
  signature = priv.sign(tree.suite, sign_label::group_info, to_be_signed());
}

bool
GroupInfo::verify(const TreeKEMPublicKey& tree) const
{
  auto maybe_leaf = tree.leaf_node(signer);
  if (!maybe_leaf) {
    throw InvalidParameterError("Signer not found");
  }

  const auto& leaf = opt::get(maybe_leaf);
  return verify(leaf.signature_key);
}

void
GroupInfo::sign(LeafIndex signer_index, const SignaturePrivateKey& priv)
{
  signer = signer_index;
  signature = priv.sign(
    group_context.cipher_suite, sign_label::group_info, to_be_signed());
}

bool
GroupInfo::verify(const SignaturePublicKey& pub) const
{
  return pub.verify(group_context.cipher_suite,
                    sign_label::group_info,
                    to_be_signed(),
                    signature);
}

// Welcome

Welcome::Welcome()
  : cipher_suite(CipherSuite::ID::unknown)
{
}

Welcome::Welcome(CipherSuite suite,
                 const bytes& joiner_secret,
                 const std::vector<PSKWithSecret>& psks,
                 const GroupInfo& group_info)
  : cipher_suite(suite)
  , _joiner_secret(joiner_secret)
{
  // Cache the list of PSK IDs
  for (const auto& psk : psks) {
    _psks.psks.push_back(psk.id);
  }

  // Pre-encrypt the GroupInfo
  auto [key, nonce] = group_info_key_nonce(suite, joiner_secret, psks);
  auto group_info_data = tls::marshal(group_info);
  encrypted_group_info =
    cipher_suite.hpke().aead.seal(key, nonce, {}, group_info_data);
}

std::optional<int>
Welcome::find(const KeyPackage& kp) const
{
  auto ref = kp.ref();
  for (size_t i = 0; i < secrets.size(); i++) {
    if (ref == secrets[i].new_member) {
      return static_cast<int>(i);
    }
  }
  return std::nullopt;
}

void
Welcome::encrypt(const KeyPackage& kp, const std::optional<bytes>& path_secret)
{
  auto gs = GroupSecrets{ _joiner_secret, std::nullopt, _psks };
  if (path_secret) {
    gs.path_secret = GroupSecrets::PathSecret{ opt::get(path_secret) };
  }

  auto gs_data = tls::marshal(gs);
  auto enc_gs = kp.init_key.encrypt(
    kp.cipher_suite, encrypt_label::welcome, encrypted_group_info, gs_data);
  secrets.push_back({ kp.ref(), enc_gs });
}

GroupSecrets
Welcome::decrypt_secrets(int kp_index, const HPKEPrivateKey& init_priv) const
{
  auto secrets_data =
    init_priv.decrypt(cipher_suite,
                      encrypt_label::welcome,
                      encrypted_group_info,
                      secrets.at(kp_index).encrypted_group_secrets);
  return tls::get<GroupSecrets>(secrets_data);
}

GroupInfo
Welcome::decrypt(const bytes& joiner_secret,
                 const std::vector<PSKWithSecret>& psks) const
{
  auto [key, nonce] = group_info_key_nonce(cipher_suite, joiner_secret, psks);
  auto group_info_data =
    cipher_suite.hpke().aead.open(key, nonce, {}, encrypted_group_info);
  if (!group_info_data) {
    throw ProtocolError("Welcome decryption failed");
  }

  return tls::get<GroupInfo>(opt::get(group_info_data));
}

KeyAndNonce
Welcome::group_info_key_nonce(CipherSuite suite,
                              const bytes& joiner_secret,
                              const std::vector<PSKWithSecret>& psks)
{
  auto welcome_secret =
    KeyScheduleEpoch::welcome_secret(suite, joiner_secret, psks);

  // XXX(RLB): These used to be done with ExpandWithLabel.  Should we do that
  // instead, for better domain separation? (In particular, including "mls10")
  // That is what we do for the sender data key/nonce.
  auto key =
    suite.expand_with_label(welcome_secret, "key", {}, suite.key_size());
  auto nonce =
    suite.expand_with_label(welcome_secret, "nonce", {}, suite.nonce_size());
  return { std::move(key), std::move(nonce) };
}

// Commit
std::optional<bytes>
Commit::valid_external() const
{
  // External Commits MUST contain a path field (and is therefore a "full"
  // Commit). The joiner is added at the leftmost free leaf node (just as if
  // they were added with an Add proposal), and the path is calculated relative
  // to that leaf node.
  //
  // The Commit MUST NOT include any proposals by reference, since an external
  // joiner cannot determine the validity of proposals sent within the group
  const auto all_by_value = stdx::all_of(proposals, [](const auto& p) {
    return var::holds_alternative<Proposal>(p.content);
  });
  if (!path || !all_by_value) {
    return std::nullopt;
  }

  const auto ext_init_ptr = stdx::find_if(proposals, [](const auto& p) {
    const auto proposal = var::get<Proposal>(p.content);
    return proposal.proposal_type() == ProposalType::external_init;
  });
  if (ext_init_ptr == proposals.end()) {
    return std::nullopt;
  }

  const auto& ext_init_proposal = var::get<Proposal>(ext_init_ptr->content);
  const auto& ext_init = var::get<ExternalInit>(ext_init_proposal.content);
  return ext_init.kem_output;
}

// PublicMessage
Proposal::Type
Proposal::proposal_type() const
{
  return tls::variant<ProposalType>::type(content).val;
}

SenderType
Sender::sender_type() const
{
  return tls::variant<SenderType>::type(sender);
}

tls::ostream&
operator<<(tls::ostream& str, const GroupContentAuthData& obj)
{
  switch (obj.content_type) {
    case ContentType::proposal:
    case ContentType::application:
      return str << obj.signature;

    case ContentType::commit:
      return str << obj.signature << opt::get(obj.confirmation_tag);

    default:
      throw InvalidParameterError("Invalid content type");
  }
}

tls::istream&
operator>>(tls::istream& str, GroupContentAuthData& obj)
{
  switch (obj.content_type) {
    case ContentType::proposal:
    case ContentType::application:
      return str >> obj.signature;

    case ContentType::commit:
      obj.confirmation_tag.emplace();
      return str >> obj.signature >> opt::get(obj.confirmation_tag);

    default:
      throw InvalidParameterError("Invalid content type");
  }
}

bool
operator==(const GroupContentAuthData& lhs, const GroupContentAuthData& rhs)
{
  return lhs.content_type == rhs.content_type &&
         lhs.signature == rhs.signature &&
         lhs.confirmation_tag == rhs.confirmation_tag;
}

GroupContent::GroupContent(bytes group_id_in,
                           epoch_t epoch_in,
                           Sender sender_in,
                           bytes authenticated_data_in,
                           RawContent content_in)
  : group_id(std::move(group_id_in))
  , epoch(epoch_in)
  , sender(sender_in)
  , authenticated_data(std::move(authenticated_data_in))
  , content(std::move(content_in))
{
}

GroupContent::GroupContent(bytes group_id_in,
                           epoch_t epoch_in,
                           Sender sender_in,
                           bytes authenticated_data_in,
                           ContentType content_type)
  : group_id(std::move(group_id_in))
  , epoch(epoch_in)
  , sender(sender_in)
  , authenticated_data(std::move(authenticated_data_in))
{
  switch (content_type) {
    case ContentType::commit:
      content.emplace<Commit>();
      break;

    case ContentType::proposal:
      content.emplace<Proposal>();
      break;

    case ContentType::application:
      content.emplace<ApplicationData>();
      break;

    default:
      throw InvalidParameterError("Invalid content type");
  }
}

ContentType
GroupContent::content_type() const
{
  return tls::variant<ContentType>::type(content);
}

AuthenticatedContent
AuthenticatedContent::sign(WireFormat wire_format,
                           GroupContent content,
                           CipherSuite suite,
                           const SignaturePrivateKey& sig_priv,
                           const std::optional<GroupContext>& context)
{
  if (wire_format == WireFormat::mls_public_message &&
      content.content_type() == ContentType::application) {
    throw InvalidParameterError(
      "Application data cannot be sent as PublicMessage");
  }

  auto content_auth = AuthenticatedContent{ wire_format, std::move(content) };
  auto tbs = content_auth.to_be_signed(context);
  content_auth.auth.signature =
    sig_priv.sign(suite, sign_label::mls_content, tbs);
  return content_auth;
}

bool
AuthenticatedContent::verify(CipherSuite suite,
                             const SignaturePublicKey& sig_pub,
                             const std::optional<GroupContext>& context) const
{
  if (wire_format == WireFormat::mls_public_message &&
      content.content_type() == ContentType::application) {
    return false;
  }

  auto tbs = to_be_signed(context);
  return sig_pub.verify(suite, sign_label::mls_content, tbs, auth.signature);
}

struct ConfirmedTranscriptHashInput
{
  WireFormat wire_format;
  const GroupContent& content;
  const bytes& signature;

  TLS_SERIALIZABLE(wire_format, content, signature);
};

struct InterimTranscriptHashInput
{
  const bytes& confirmation_tag;

  TLS_SERIALIZABLE(confirmation_tag);
};

bytes
AuthenticatedContent::confirmed_transcript_hash_input() const
{
  return tls::marshal(ConfirmedTranscriptHashInput{
    wire_format,
    content,
    auth.signature,
  });
}

bytes
AuthenticatedContent::interim_transcript_hash_input() const
{
  return tls::marshal(
    InterimTranscriptHashInput{ opt::get(auth.confirmation_tag) });
}

void
AuthenticatedContent::set_confirmation_tag(const bytes& confirmation_tag)
{
  auth.confirmation_tag = confirmation_tag;
}

bool
AuthenticatedContent::check_confirmation_tag(
  const bytes& confirmation_tag) const
{
  return confirmation_tag == opt::get(auth.confirmation_tag);
}

tls::ostream&
operator<<(tls::ostream& str, const AuthenticatedContent& obj)
{
  return str << obj.wire_format << obj.content << obj.auth;
}

tls::istream&
operator>>(tls::istream& str, AuthenticatedContent& obj)
{
  str >> obj.wire_format >> obj.content;

  obj.auth.content_type = obj.content.content_type();
  return str >> obj.auth;
}

bool
operator==(const AuthenticatedContent& lhs, const AuthenticatedContent& rhs)
{
  return lhs.wire_format == rhs.wire_format && lhs.content == rhs.content &&
         lhs.auth == rhs.auth;
}

AuthenticatedContent::AuthenticatedContent(WireFormat wire_format_in,
                                           GroupContent content_in)
  : wire_format(wire_format_in)
  , content(std::move(content_in))
{
  auth.content_type = content.content_type();
}

AuthenticatedContent::AuthenticatedContent(WireFormat wire_format_in,
                                           GroupContent content_in,
                                           GroupContentAuthData auth_in)
  : wire_format(wire_format_in)
  , content(std::move(content_in))
  , auth(std::move(auth_in))
{
}

const AuthenticatedContent&
ValidatedContent::authenticated_content() const
{
  return content_auth;
}

ValidatedContent::ValidatedContent(AuthenticatedContent content_auth_in)
  : content_auth(std::move(content_auth_in))
{
}

bool
operator==(const ValidatedContent& lhs, const ValidatedContent& rhs)
{
  return lhs.content_auth == rhs.content_auth;
}

struct GroupContentTBS
{
  WireFormat wire_format = WireFormat::reserved;
  const GroupContent& content;
  const std::optional<GroupContext>& context;
};

static tls::ostream&
operator<<(tls::ostream& str, const GroupContentTBS& obj)
{
  str << ProtocolVersion::mls10 << obj.wire_format << obj.content;

  switch (obj.content.sender.sender_type()) {
    case SenderType::member:
    case SenderType::new_member_commit:
      str << opt::get(obj.context);
      break;

    case SenderType::external:
    case SenderType::new_member_proposal:
      break;

    default:
      throw InvalidParameterError("Invalid sender type");
  }

  return str;
}

bytes
AuthenticatedContent::to_be_signed(
  const std::optional<GroupContext>& context) const
{
  return tls::marshal(GroupContentTBS{
    wire_format,
    content,
    context,
  });
}

PublicMessage
PublicMessage::protect(AuthenticatedContent content_auth,
                       CipherSuite suite,
                       const std::optional<bytes>& membership_key,
                       const std::optional<GroupContext>& context)
{
  auto pt = PublicMessage(std::move(content_auth));

  // Add the membership_mac if required
  switch (pt.content.sender.sender_type()) {
    case SenderType::member:
      pt.membership_tag =
        pt.membership_mac(suite, opt::get(membership_key), context);
      break;

    default:
      break;
  }

  return pt;
}

std::optional<ValidatedContent>
PublicMessage::unprotect(CipherSuite suite,
                         const std::optional<bytes>& membership_key,
                         const std::optional<GroupContext>& context) const
{
  // Verify the membership_tag if the message was sent within the group
  switch (content.sender.sender_type()) {
    case SenderType::member: {
      auto candidate = membership_mac(suite, opt::get(membership_key), context);
      if (candidate != opt::get(membership_tag)) {
        return std::nullopt;
      }
      break;
    }

    default:
      break;
  }

  return { { AuthenticatedContent{
    WireFormat::mls_public_message,
    content,
    auth,
  } } };
}

bool
PublicMessage::contains(const AuthenticatedContent& content_auth) const
{
  return content == content_auth.content && auth == content_auth.auth;
}

AuthenticatedContent
PublicMessage::authenticated_content() const
{
  auto auth_content = AuthenticatedContent{};
  auth_content.wire_format = WireFormat::mls_public_message;
  auth_content.content = content;
  auth_content.auth = auth;
  return auth_content;
}

PublicMessage::PublicMessage(AuthenticatedContent content_auth)
  : content(std::move(content_auth.content))
  , auth(std::move(content_auth.auth))
{
  if (content_auth.wire_format != WireFormat::mls_public_message) {
    throw InvalidParameterError("Wire format mismatch (not mls_plaintext)");
  }
}

struct GroupContentTBM
{
  GroupContentTBS content_tbs;
  GroupContentAuthData auth;

  TLS_SERIALIZABLE(content_tbs, auth);
};

bytes
PublicMessage::membership_mac(CipherSuite suite,
                              const bytes& membership_key,
                              const std::optional<GroupContext>& context) const
{
  auto tbm = tls::marshal(GroupContentTBM{
    { WireFormat::mls_public_message, content, context },
    auth,
  });

  return suite.digest().hmac(membership_key, tbm);
}

tls::ostream&
operator<<(tls::ostream& str, const PublicMessage& obj)
{
  switch (obj.content.sender.sender_type()) {
    case SenderType::member:
      return str << obj.content << obj.auth << opt::get(obj.membership_tag);

    case SenderType::external:
    case SenderType::new_member_proposal:
    case SenderType::new_member_commit:
      return str << obj.content << obj.auth;

    default:
      throw InvalidParameterError("Invalid sender type");
  }
}

tls::istream&
operator>>(tls::istream& str, PublicMessage& obj)
{
  str >> obj.content;

  obj.auth.content_type = obj.content.content_type();
  str >> obj.auth;

  if (obj.content.sender.sender_type() == SenderType::member) {
    obj.membership_tag.emplace();
    str >> opt::get(obj.membership_tag);
  }

  return str;
}

bool
operator==(const PublicMessage& lhs, const PublicMessage& rhs)
{
  return lhs.content == rhs.content && lhs.auth == rhs.auth &&
         lhs.membership_tag == rhs.membership_tag;
}

bool
operator!=(const PublicMessage& lhs, const PublicMessage& rhs)
{
  return !(lhs == rhs);
}

static bytes
marshal_ciphertext_content(const GroupContent& content,
                           const GroupContentAuthData& auth,
                           size_t padding_size)
{
  auto w = tls::ostream{};
  var::visit([&w](const auto& val) { w << val; }, content.content);
  w << auth;
  w.write_raw(bytes(padding_size, 0));
  return w.bytes();
}

static void
unmarshal_ciphertext_content(const bytes& content_pt,
                             GroupContent& content,
                             GroupContentAuthData& auth)
{
  auto r = tls::istream(content_pt);

  var::visit([&r](auto& val) { r >> val; }, content.content);
  r >> auth;

  const auto padding = r.bytes();
  const auto nonzero = [](const auto& x) { return x != 0; };
  if (stdx::any_of(padding, nonzero)) {
    throw ProtocolError("Malformed AuthenticatedContentTBE padding");
  }
}

struct ContentAAD
{
  const bytes& group_id;
  const epoch_t epoch;
  const ContentType content_type;
  const bytes& authenticated_data;

  TLS_SERIALIZABLE(group_id, epoch, content_type, authenticated_data)
};

struct SenderData
{
  LeafIndex sender{ 0 };
  uint32_t generation{ 0 };
  ReuseGuard reuse_guard{ 0, 0, 0, 0 };

  TLS_SERIALIZABLE(sender, generation, reuse_guard)
};

struct SenderDataAAD
{
  const bytes& group_id;
  const epoch_t epoch;
  const ContentType content_type;

  TLS_SERIALIZABLE(group_id, epoch, content_type)
};

PrivateMessage
PrivateMessage::protect(AuthenticatedContent content_auth,
                        CipherSuite suite,
                        GroupKeySource& keys,
                        const bytes& sender_data_secret,
                        size_t padding_size)
{
  // Pull keys from the secret tree
  auto index =
    var::get<MemberSender>(content_auth.content.sender.sender).sender;
  auto content_type = content_auth.content.content_type();
  auto [generation, reuse_guard, content_keys] = keys.next(content_type, index);

  // Encrypt the content
  auto content_pt = marshal_ciphertext_content(
    content_auth.content, content_auth.auth, padding_size);
  auto content_aad = tls::marshal(ContentAAD{
    content_auth.content.group_id,
    content_auth.content.epoch,
    content_auth.content.content_type(),
    content_auth.content.authenticated_data,
  });

  auto content_ct = suite.hpke().aead.seal(
    content_keys.key, content_keys.nonce, content_aad, content_pt);

  // Encrypt the sender data
  auto sender_index =
    var::get<MemberSender>(content_auth.content.sender.sender).sender;
  auto sender_data_pt = tls::marshal(SenderData{
    sender_index,
    generation,
    reuse_guard,
  });
  auto sender_data_aad = tls::marshal(SenderDataAAD{
    content_auth.content.group_id,
    content_auth.content.epoch,
    content_auth.content.content_type(),
  });

  auto sender_data_keys =
    KeyScheduleEpoch::sender_data_keys(suite, sender_data_secret, content_ct);

  auto sender_data_ct = suite.hpke().aead.seal(sender_data_keys.key,
                                               sender_data_keys.nonce,
                                               sender_data_aad,
                                               sender_data_pt);

  return PrivateMessage{
    std::move(content_auth.content),
    std::move(sender_data_ct),
    std::move(content_ct),
  };
}

std::optional<ValidatedContent>
PrivateMessage::unprotect(CipherSuite suite,
                          GroupKeySource& keys,
                          const bytes& sender_data_secret) const
{
  // Decrypt and parse the sender data
  auto sender_data_keys =
    KeyScheduleEpoch::sender_data_keys(suite, sender_data_secret, ciphertext);
  auto sender_data_aad = tls::marshal(SenderDataAAD{
    group_id,
    epoch,
    content_type,
  });

  auto sender_data_pt = suite.hpke().aead.open(sender_data_keys.key,
                                               sender_data_keys.nonce,
                                               sender_data_aad,
                                               encrypted_sender_data);
  if (!sender_data_pt) {
    return std::nullopt;
  }

  auto sender_data = tls::get<SenderData>(opt::get(sender_data_pt));
  if (!keys.has_leaf(sender_data.sender)) {
    return std::nullopt;
  }

  // Decrypt the content
  auto content_keys = keys.get(content_type,
                               sender_data.sender,
                               sender_data.generation,
                               sender_data.reuse_guard);
  keys.erase(content_type, sender_data.sender, sender_data.generation);

  auto content_aad = tls::marshal(ContentAAD{
    group_id,
    epoch,
    content_type,
    authenticated_data,
  });

  auto content_pt = suite.hpke().aead.open(
    content_keys.key, content_keys.nonce, content_aad, ciphertext);
  if (!content_pt) {
    return std::nullopt;
  }

  // Parse the content
  auto content = GroupContent{ group_id,
                               epoch,
                               { MemberSender{ sender_data.sender } },
                               authenticated_data,
                               content_type };
  auto auth = GroupContentAuthData{ content_type, {}, {} };

  unmarshal_ciphertext_content(opt::get(content_pt), content, auth);

  return { { AuthenticatedContent{
    WireFormat::mls_private_message,
    std::move(content),
    std::move(auth),
  } } };
}

PrivateMessage::PrivateMessage(GroupContent content,
                               bytes encrypted_sender_data_in,
                               bytes ciphertext_in)
  : group_id(std::move(content.group_id))
  , epoch(content.epoch)
  , content_type(content.content_type())
  , authenticated_data(std::move(content.authenticated_data))
  , encrypted_sender_data(std::move(encrypted_sender_data_in))
  , ciphertext(std::move(ciphertext_in))
{
}

bytes
MLSMessage::group_id() const
{
  return var::visit(
    overloaded{
      [](const PublicMessage& pt) -> bytes { return pt.get_group_id(); },
      [](const PrivateMessage& ct) -> bytes { return ct.get_group_id(); },
      [](const GroupInfo& gi) -> bytes { return gi.group_context.group_id; },
      [](const auto& /* unused */) -> bytes {
        throw InvalidParameterError("MLSMessage has no group_id");
      },
    },
    message);
}

epoch_t
MLSMessage::epoch() const
{
  return var::visit(
    overloaded{
      [](const PublicMessage& pt) -> epoch_t { return pt.get_epoch(); },
      [](const PrivateMessage& pt) -> epoch_t { return pt.get_epoch(); },
      [](const auto& /* unused */) -> epoch_t {
        throw InvalidParameterError("MLSMessage has no epoch");
      },
    },
    message);
}

WireFormat
MLSMessage::wire_format() const
{
  return tls::variant<WireFormat>::type(message);
}

MLSMessage::MLSMessage(PublicMessage public_message)
  : message(std::move(public_message))
{
}

MLSMessage::MLSMessage(PrivateMessage private_message)
  : message(std::move(private_message))
{
}

MLSMessage::MLSMessage(Welcome welcome)
  : message(std::move(welcome))
{
}

MLSMessage::MLSMessage(GroupInfo group_info)
  : message(std::move(group_info))
{
}

MLSMessage::MLSMessage(KeyPackage key_package)
  : message(std::move(key_package))
{
}

MLSMessage
external_proposal(CipherSuite suite,
                  const bytes& group_id,
                  epoch_t epoch,
                  const Proposal& proposal,
                  uint32_t signer_index,
                  const SignaturePrivateKey& sig_priv)
{
  switch (proposal.proposal_type()) {
    // These proposal types are OK
    case ProposalType::add:
    case ProposalType::remove:
    case ProposalType::psk:
    case ProposalType::reinit:
    case ProposalType::group_context_extensions:
      break;

    // These proposal types are forbidden
    case ProposalType::invalid:
    case ProposalType::update:
    case ProposalType::external_init:
    default:
      throw ProtocolError("External proposal has invalid type");
  }

  auto content = GroupContent{ group_id,
                               epoch,
                               { ExternalSenderIndex{ signer_index } },
                               { /* no authenticated data */ },
                               { proposal } };
  auto content_auth = AuthenticatedContent::sign(
    WireFormat::mls_public_message, std::move(content), suite, sig_priv, {});

  return PublicMessage::protect(std::move(content_auth), suite, {}, {});
}

} // namespace mlspp
