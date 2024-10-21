#include "mls/core_types.h"
#include "mls/messages.h"

#include "grease.h"

#include <set>

namespace mlspp {

///
/// Extensions
///

const Extension::Type RequiredCapabilitiesExtension::type =
  ExtensionType::required_capabilities;
const Extension::Type ApplicationIDExtension::type =
  ExtensionType::application_id;

const std::array<uint16_t, 5> default_extensions = {
  ExtensionType::application_id,        ExtensionType::ratchet_tree,
  ExtensionType::required_capabilities, ExtensionType::external_pub,
  ExtensionType::external_senders,
};

const std::array<uint16_t, 8> default_proposals = {
  ProposalType::add,
  ProposalType::update,
  ProposalType::remove,
  ProposalType::psk,
  ProposalType::reinit,
  ProposalType::external_init,
  ProposalType::group_context_extensions,
};

const std::array<ProtocolVersion, 1> all_supported_versions = {
  ProtocolVersion::mls10
};

const std::array<CipherSuite::ID, 6> all_supported_ciphersuites = {
  CipherSuite::ID::X25519_AES128GCM_SHA256_Ed25519,
  CipherSuite::ID::P256_AES128GCM_SHA256_P256,
  CipherSuite::ID::X25519_CHACHA20POLY1305_SHA256_Ed25519,
  CipherSuite::ID::X448_AES256GCM_SHA512_Ed448,
  CipherSuite::ID::P521_AES256GCM_SHA512_P521,
  CipherSuite::ID::X448_CHACHA20POLY1305_SHA512_Ed448,
};

const std::array<CredentialType, 4> all_supported_credentials = {
  CredentialType::basic,
  CredentialType::x509,
  CredentialType::userinfo_vc_draft_00,
  CredentialType::multi_draft_00
};

Capabilities
Capabilities::create_default()
{
  return {
    { all_supported_versions.begin(), all_supported_versions.end() },
    { all_supported_ciphersuites.begin(), all_supported_ciphersuites.end() },
    { /* No non-default extensions */ },
    { /* No non-default proposals */ },
    { all_supported_credentials.begin(), all_supported_credentials.end() },
  };
}

bool
Capabilities::extensions_supported(
  const std::vector<Extension::Type>& required) const
{
  return stdx::all_of(required, [&](Extension::Type type) {
    if (stdx::contains(default_extensions, type)) {
      return true;
    }

    return stdx::contains(extensions, type);
  });
}

bool
Capabilities::proposals_supported(
  const std::vector<Proposal::Type>& required) const
{
  return stdx::all_of(required, [&](Proposal::Type type) {
    if (stdx::contains(default_proposals, type)) {
      return true;
    }

    return stdx::contains(proposals, type);
  });
}

bool
Capabilities::credential_supported(const Credential& credential) const
{
  return stdx::contains(credentials, credential.type());
}

Lifetime
Lifetime::create_default()
{
  return Lifetime{ 0x0000000000000000, 0xffffffffffffffff };
}

void
ExtensionList::add(uint16_t type, bytes data)
{
  auto curr = stdx::find_if(
    extensions, [&](const Extension& ext) -> bool { return ext.type == type; });
  if (curr != extensions.end()) {
    curr->data = std::move(data);
    return;
  }

  extensions.push_back({ type, std::move(data) });
}

bool
ExtensionList::has(uint16_t type) const
{
  return stdx::any_of(extensions,
                      [&](const Extension& ext) { return ext.type == type; });
}

///
/// LeafNode
///
LeafNode::LeafNode(CipherSuite cipher_suite,
                   HPKEPublicKey encryption_key_in,
                   SignaturePublicKey signature_key_in,
                   Credential credential_in,
                   Capabilities capabilities_in,
                   Lifetime lifetime_in,
                   ExtensionList extensions_in,
                   const SignaturePrivateKey& sig_priv)
  : encryption_key(std::move(encryption_key_in))
  , signature_key(std::move(signature_key_in))
  , credential(std::move(credential_in))
  , capabilities(std::move(capabilities_in))
  , content(lifetime_in)
  , extensions(std::move(extensions_in))
{
  grease(extensions);
  grease(capabilities, extensions);
  sign(cipher_suite, sig_priv, std::nullopt);
}

void
LeafNode::set_capabilities(Capabilities capabilities_in)
{
  capabilities = std::move(capabilities_in);
  grease(capabilities, extensions);
}

LeafNode
LeafNode::for_update(CipherSuite cipher_suite,
                     const bytes& group_id,
                     LeafIndex leaf_index,
                     HPKEPublicKey encryption_key_in,
                     const LeafNodeOptions& opts,
                     const SignaturePrivateKey& sig_priv) const
{
  auto clone = clone_with_options(std::move(encryption_key_in), opts);

  clone.content = Empty{};
  clone.sign(cipher_suite, sig_priv, { { group_id, leaf_index } });

  return clone;
}

LeafNode
LeafNode::for_commit(CipherSuite cipher_suite,
                     const bytes& group_id,
                     LeafIndex leaf_index,
                     HPKEPublicKey encryption_key_in,
                     const bytes& parent_hash,
                     const LeafNodeOptions& opts,
                     const SignaturePrivateKey& sig_priv) const
{
  auto clone = clone_with_options(std::move(encryption_key_in), opts);

  clone.content = ParentHash{ parent_hash };
  clone.sign(cipher_suite, sig_priv, { { group_id, leaf_index } });

  return clone;
}

LeafNodeSource
LeafNode::source() const
{
  return tls::variant<LeafNodeSource>::type(content);
}

void
LeafNode::sign(CipherSuite cipher_suite,
               const SignaturePrivateKey& sig_priv,
               const std::optional<MemberBinding>& binding)
{
  const auto tbs = to_be_signed(binding);

  if (sig_priv.public_key != signature_key) {
    throw InvalidParameterError("Signature key mismatch");
  }

  if (!credential.valid_for(signature_key)) {
    throw InvalidParameterError("Credential not valid for signature key");
  }

  signature = sig_priv.sign(cipher_suite, sign_label::leaf_node, tbs);
}

bool
LeafNode::verify(CipherSuite cipher_suite,
                 const std::optional<MemberBinding>& binding) const
{
  const auto tbs = to_be_signed(binding);

  if (CredentialType::x509 == credential.type()) {
    const auto& cred = credential.get<X509Credential>();
    if (cred.signature_scheme() !=
        tls_signature_scheme(cipher_suite.sig().id)) {
      throw std::runtime_error("Signature algorithm invalid");
    }
  }

  return signature_key.verify(
    cipher_suite, sign_label::leaf_node, tbs, signature);
}

bool
LeafNode::verify_expiry(uint64_t now) const
{
  const auto valid = overloaded{
    [now](const Lifetime& lt) {
      return lt.not_before <= now && now <= lt.not_after;
    },
    [](const auto& /* other */) { return false; },
  };
  return var::visit(valid, content);
}

bool
LeafNode::verify_extension_support(const ExtensionList& ext_list) const
{
  // Verify that extensions in the list are supported
  auto ext_types = stdx::transform<Extension::Type>(
    ext_list.extensions, [](const auto& ext) { return ext.type; });

  if (!capabilities.extensions_supported(ext_types)) {
    return false;
  }

  // If there's a RequiredCapabilities extension, verify support
  const auto maybe_req_capas = ext_list.find<RequiredCapabilitiesExtension>();
  if (!maybe_req_capas) {
    return true;
  }

  const auto& req_capas = opt::get(maybe_req_capas);
  return capabilities.extensions_supported(req_capas.extensions) &&
         capabilities.proposals_supported(req_capas.proposals);
}

LeafNode
LeafNode::clone_with_options(HPKEPublicKey encryption_key_in,
                             const LeafNodeOptions& opts) const
{
  auto clone = *this;

  clone.encryption_key = std::move(encryption_key_in);

  if (opts.credential) {
    clone.credential = opt::get(opts.credential);
  }

  if (opts.capabilities) {
    clone.capabilities = opt::get(opts.capabilities);
  }

  if (opts.extensions) {
    clone.extensions = opt::get(opts.extensions);
  }

  return clone;
}

// struct {
//     HPKEPublicKey encryption_key;
//     SignaturePublicKey signature_key;
//     Credential credential;
//     Capabilities capabilities;
//
//     LeafNodeSource leaf_node_source;
//     select (leaf_node_source) {
//         case key_package:
//             Lifetime lifetime;
//
//         case update:
//             struct{};
//
//         case commit:
//             opaque parent_hash<V>;
//     }
//
//     Extension extensions<V>;
//
//     select (leaf_node_source) {
//         case key_package:
//             struct{};
//
//         case update:
//             opaque group_id<V>;
//
//         case commit:
//             opaque group_id<V>;
//     }
// } LeafNodeTBS;
struct LeafNodeTBS
{
  const HPKEPublicKey& encryption_key;
  const SignaturePublicKey& signature_key;
  const Credential& credential;
  const Capabilities& capabilities;
  const var::variant<Lifetime, Empty, ParentHash>& content;
  const ExtensionList& extensions;

  TLS_SERIALIZABLE(encryption_key,
                   signature_key,
                   credential,
                   capabilities,
                   content,
                   extensions)
  TLS_TRAITS(tls::pass,
             tls::pass,
             tls::pass,
             tls::pass,
             tls::variant<LeafNodeSource>,
             tls::pass)
};

bytes
LeafNode::to_be_signed(const std::optional<MemberBinding>& binding) const
{
  tls::ostream w;

  w << LeafNodeTBS{
    encryption_key, signature_key, credential,
    capabilities,   content,       extensions,
  };

  switch (source()) {
    case LeafNodeSource::key_package:
      break;

    case LeafNodeSource::update:
    case LeafNodeSource::commit:
      w << opt::get(binding);
  }

  return w.bytes();
}

///
/// NodeType, ParentNode, and KeyPackage
///

bytes
ParentNode::hash(CipherSuite suite) const
{
  return suite.digest().hash(tls::marshal(this));
}

KeyPackage::KeyPackage()
  : version(ProtocolVersion::mls10)
  , cipher_suite(CipherSuite::ID::unknown)
{
}

KeyPackage::KeyPackage(CipherSuite suite_in,
                       HPKEPublicKey init_key_in,
                       LeafNode leaf_node_in,
                       ExtensionList extensions_in,
                       const SignaturePrivateKey& sig_priv_in)
  : version(ProtocolVersion::mls10)
  , cipher_suite(suite_in)
  , init_key(std::move(init_key_in))
  , leaf_node(std::move(leaf_node_in))
  , extensions(std::move(extensions_in))
{
  grease(extensions);
  sign(sig_priv_in);
}

KeyPackageRef
KeyPackage::ref() const
{
  return cipher_suite.ref(*this);
}

void
KeyPackage::sign(const SignaturePrivateKey& sig_priv)
{
  auto tbs = to_be_signed();
  signature = sig_priv.sign(cipher_suite, sign_label::key_package, tbs);
}

bool
KeyPackage::verify() const
{
  // Verify the inner leaf node
  if (!leaf_node.verify(cipher_suite, std::nullopt)) {
    return false;
  }

  // Check that the inner leaf node is intended for use in a KeyPackage
  if (leaf_node.source() != LeafNodeSource::key_package) {
    return false;
  }

  // Verify the KeyPackage
  const auto tbs = to_be_signed();

  if (CredentialType::x509 == leaf_node.credential.type()) {
    const auto& cred = leaf_node.credential.get<X509Credential>();
    if (cred.signature_scheme() !=
        tls_signature_scheme(cipher_suite.sig().id)) {
      throw std::runtime_error("Signature algorithm invalid");
    }
  }

  return leaf_node.signature_key.verify(
    cipher_suite, sign_label::key_package, tbs, signature);
}

bytes
KeyPackage::to_be_signed() const
{
  tls::ostream out;
  out << version << cipher_suite << init_key << leaf_node << extensions;
  return out.bytes();
}

} // namespace mlspp
