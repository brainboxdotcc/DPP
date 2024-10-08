#pragma once

#include "mls/credential.h"
#include "mls/crypto.h"
#include "mls/tree_math.h"

namespace mlspp {

// enum {
//   reserved(0),
//   mls10(1),
//   (255)
// } ProtocolVersion;
enum class ProtocolVersion : uint16_t
{
  mls10 = 0x01,
};

extern const std::array<ProtocolVersion, 1> all_supported_versions;

// struct {
//     ExtensionType extension_type;
//     opaque extension_data<V>;
// } Extension;
struct Extension
{
  using Type = uint16_t;

  Type type;
  bytes data;

  TLS_SERIALIZABLE(type, data)
};

struct ExtensionType
{
  static constexpr Extension::Type application_id = 1;
  static constexpr Extension::Type ratchet_tree = 2;
  static constexpr Extension::Type required_capabilities = 3;
  static constexpr Extension::Type external_pub = 4;
  static constexpr Extension::Type external_senders = 5;

  // XXX(RLB) There is no IANA-registered type for this extension yet, so we use
  // a value from the vendor-specific space
  static constexpr Extension::Type sframe_parameters = 0xff02;
};

struct ExtensionList
{
  std::vector<Extension> extensions;

  // XXX(RLB) It would be good if this maintained extensions in order.  It might
  // be possible to do this automatically by changing the storage to a
  // map<ExtensionType, bytes> and extending the TLS code to marshal that type.
  template<typename T>
  inline void add(const T& obj)
  {
    auto data = tls::marshal(obj);
    add(T::type, std::move(data));
  }

  void add(Extension::Type type, bytes data);

  template<typename T>
  std::optional<T> find() const
  {
    for (const auto& ext : extensions) {
      if (ext.type == T::type) {
        return tls::get<T>(ext.data);
      }
    }

    return std::nullopt;
  }

  bool has(uint16_t type) const;

  TLS_SERIALIZABLE(extensions)
};

// enum {
//     reserved(0),
//     key_package(1),
//     update(2),
//     commit(3),
//     (255)
// } LeafNodeSource;
enum struct LeafNodeSource : uint8_t
{
  key_package = 1,
  update = 2,
  commit = 3,
};

// struct {
//     ProtocolVersion versions<V>;
//     CipherSuite ciphersuites<V>;
//     ExtensionType extensions<V>;
//     ProposalType proposals<V>;
//     CredentialType credentials<V>;
// } Capabilities;
struct Capabilities
{
  std::vector<ProtocolVersion> versions;
  std::vector<CipherSuite::ID> cipher_suites;
  std::vector<Extension::Type> extensions;
  std::vector<uint16_t> proposals;
  std::vector<CredentialType> credentials;

  static Capabilities create_default();
  bool extensions_supported(const std::vector<Extension::Type>& required) const;
  bool proposals_supported(const std::vector<uint16_t>& required) const;
  bool credential_supported(const Credential& credential) const;

  template<typename Container>
  bool credentials_supported(const Container& required) const
  {
    return stdx::all_of(required, [&](CredentialType type) {
      return stdx::contains(credentials, type);
    });
  }

  TLS_SERIALIZABLE(versions, cipher_suites, extensions, proposals, credentials)
};

// struct {
//     uint64 not_before;
//     uint64 not_after;
// } Lifetime;
struct Lifetime
{
  uint64_t not_before;
  uint64_t not_after;

  static Lifetime create_default();

  TLS_SERIALIZABLE(not_before, not_after)
};

// struct {
//     HPKEPublicKey encryption_key;
//     SignaturePublicKey signature_key;
//     Credential credential;
//     Capabilities capabilities;
//
//     LeafNodeSource leaf_node_source;
//     select (leaf_node_source) {
//         case add:
//             Lifetime lifetime;
//
//         case update:
//             struct {}
//
//         case commit:
//             opaque parent_hash<V>;
//     }
//
//     Extension extensions<V>;
//     // SignWithLabel(., "LeafNodeTBS", LeafNodeTBS)
//     opaque signature<V>;
// } LeafNode;
struct Empty
{
  TLS_SERIALIZABLE()
};

struct ParentHash
{
  bytes parent_hash;
  TLS_SERIALIZABLE(parent_hash);
};

struct LeafNodeOptions
{
  std::optional<Credential> credential;
  std::optional<Capabilities> capabilities;
  std::optional<ExtensionList> extensions;
};

// TODO Move this to treekem.h
struct LeafNode
{
  HPKEPublicKey encryption_key;
  SignaturePublicKey signature_key;
  Credential credential;
  Capabilities capabilities;

  var::variant<Lifetime, Empty, ParentHash> content;

  ExtensionList extensions;
  bytes signature;

  LeafNode() = default;
  LeafNode(const LeafNode&) = default;
  LeafNode(LeafNode&&) = default;
  LeafNode& operator=(const LeafNode&) = default;
  LeafNode& operator=(LeafNode&&) = default;

  LeafNode(CipherSuite cipher_suite,
           HPKEPublicKey encryption_key_in,
           SignaturePublicKey signature_key_in,
           Credential credential_in,
           Capabilities capabilities_in,
           Lifetime lifetime_in,
           ExtensionList extensions_in,
           const SignaturePrivateKey& sig_priv);

  LeafNode for_update(CipherSuite cipher_suite,
                      const bytes& group_id,
                      LeafIndex leaf_index,
                      HPKEPublicKey encryption_key,
                      const LeafNodeOptions& opts,
                      const SignaturePrivateKey& sig_priv_in) const;

  LeafNode for_commit(CipherSuite cipher_suite,
                      const bytes& group_id,
                      LeafIndex leaf_index,
                      HPKEPublicKey encryption_key,
                      const bytes& parent_hash,
                      const LeafNodeOptions& opts,
                      const SignaturePrivateKey& sig_priv_in) const;

  void set_capabilities(Capabilities capabilities_in);

  LeafNodeSource source() const;

  struct MemberBinding
  {
    bytes group_id;
    LeafIndex leaf_index;
    TLS_SERIALIZABLE(group_id, leaf_index);
  };

  void sign(CipherSuite cipher_suite,
            const SignaturePrivateKey& sig_priv,
            const std::optional<MemberBinding>& binding);
  bool verify(CipherSuite cipher_suite,
              const std::optional<MemberBinding>& binding) const;

  bool verify_expiry(uint64_t now) const;
  bool verify_extension_support(const ExtensionList& ext_list) const;

  TLS_SERIALIZABLE(encryption_key,
                   signature_key,
                   credential,
                   capabilities,
                   content,
                   extensions,
                   signature)
  TLS_TRAITS(tls::pass,
             tls::pass,
             tls::pass,
             tls::pass,
             tls::variant<LeafNodeSource>,
             tls::pass,
             tls::pass)

private:
  LeafNode clone_with_options(HPKEPublicKey encryption_key,
                              const LeafNodeOptions& opts) const;
  bytes to_be_signed(const std::optional<MemberBinding>& binding) const;
};

// Concrete extension types
struct RequiredCapabilitiesExtension
{
  std::vector<Extension::Type> extensions;
  std::vector<uint16_t> proposals;

  static const Extension::Type type;
  TLS_SERIALIZABLE(extensions, proposals)
};

struct ApplicationIDExtension
{
  bytes id;

  static const Extension::Type type;
  TLS_SERIALIZABLE(id)
};

///
/// NodeType, ParentNode, and KeyPackage
///

// TODO move this to treekem.h
struct ParentNode
{
  HPKEPublicKey public_key;
  bytes parent_hash;
  std::vector<LeafIndex> unmerged_leaves;

  bytes hash(CipherSuite suite) const;

  TLS_SERIALIZABLE(public_key, parent_hash, unmerged_leaves)
};

// TODO Move this to messages.h
// struct {
//     ProtocolVersion version;
//     CipherSuite cipher_suite;
//     HPKEPublicKey init_key;
//     LeafNode leaf_node;
//     Extension extensions<V>;
//     // SignWithLabel(., "KeyPackageTBS", KeyPackageTBS)
//     opaque signature<V>;
// } KeyPackage;
struct KeyPackage
{
  ProtocolVersion version;
  CipherSuite cipher_suite;
  HPKEPublicKey init_key;
  LeafNode leaf_node;
  ExtensionList extensions;
  bytes signature;

  KeyPackage();
  KeyPackage(CipherSuite suite_in,
             HPKEPublicKey init_key_in,
             LeafNode leaf_node_in,
             ExtensionList extensions_in,
             const SignaturePrivateKey& sig_priv_in);

  KeyPackageRef ref() const;

  void sign(const SignaturePrivateKey& sig_priv);
  bool verify() const;

  TLS_SERIALIZABLE(version,
                   cipher_suite,
                   init_key,
                   leaf_node,
                   extensions,
                   signature)

private:
  bytes to_be_signed() const;
};

///
/// UpdatePath
///

// struct {
//     HPKEPublicKey public_key;
//     HPKECiphertext encrypted_path_secret<V>;
// } UpdatePathNode;
struct UpdatePathNode
{
  HPKEPublicKey public_key;
  std::vector<HPKECiphertext> encrypted_path_secret;

  TLS_SERIALIZABLE(public_key, encrypted_path_secret)
};

// struct {
//     LeafNode leaf_node;
//     UpdatePathNode nodes<V>;
// } UpdatePath;
struct UpdatePath
{
  LeafNode leaf_node;
  std::vector<UpdatePathNode> nodes;

  TLS_SERIALIZABLE(leaf_node, nodes)
};

} // namespace mlspp

namespace mlspp::tls {

TLS_VARIANT_MAP(mlspp::LeafNodeSource,
                mlspp::Lifetime,
                key_package)
TLS_VARIANT_MAP(mlspp::LeafNodeSource, mlspp::Empty, update)
TLS_VARIANT_MAP(mlspp::LeafNodeSource,
                mlspp::ParentHash,
                commit)

} // namespace mlspp::tls
