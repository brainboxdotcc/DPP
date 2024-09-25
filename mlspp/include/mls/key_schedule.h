#pragma once

#include <map>
#include <mls/common.h>
#include <mls/crypto.h>
#include <mls/messages.h>
#include <mls/tree_math.h>

namespace mlspp {

struct HashRatchet
{
  CipherSuite suite;
  bytes next_secret;
  uint32_t next_generation;
  std::map<uint32_t, KeyAndNonce> cache;

  size_t key_size;
  size_t nonce_size;
  size_t secret_size;

  // These defaults are necessary for use with containers
  HashRatchet() = default;
  HashRatchet(const HashRatchet& other) = default;
  HashRatchet(HashRatchet&& other) = default;
  HashRatchet& operator=(const HashRatchet& other) = default;
  HashRatchet& operator=(HashRatchet&& other) = default;

  HashRatchet(CipherSuite suite_in, bytes base_secret_in);

  std::tuple<uint32_t, KeyAndNonce> next();
  KeyAndNonce get(uint32_t generation);
  void erase(uint32_t generation);
};

struct SecretTree
{
  SecretTree() = default;
  SecretTree(CipherSuite suite_in,
             LeafCount group_size_in,
             bytes encryption_secret_in);

  bool has_leaf(LeafIndex sender) { return sender < group_size; }

  bytes get(LeafIndex sender);

private:
  CipherSuite suite;
  LeafCount group_size;
  NodeIndex root;
  std::map<NodeIndex, bytes> secrets;
  size_t secret_size;
};

using ReuseGuard = std::array<uint8_t, 4>;

struct GroupKeySource
{
  enum struct RatchetType
  {
    handshake,
    application,
  };

  GroupKeySource() = default;
  GroupKeySource(CipherSuite suite_in,
                 LeafCount group_size,
                 bytes encryption_secret);

  bool has_leaf(LeafIndex sender) { return secret_tree.has_leaf(sender); }

  std::tuple<uint32_t, ReuseGuard, KeyAndNonce> next(ContentType content_type,
                                                     LeafIndex sender);
  KeyAndNonce get(ContentType content_type,
                  LeafIndex sender,
                  uint32_t generation,
                  ReuseGuard reuse_guard);
  void erase(ContentType type, LeafIndex sender, uint32_t generation);

private:
  CipherSuite suite;
  SecretTree secret_tree;

  using Key = std::tuple<RatchetType, LeafIndex>;
  std::map<Key, HashRatchet> chains;

  HashRatchet& chain(RatchetType type, LeafIndex sender);
  HashRatchet& chain(ContentType type, LeafIndex sender);

  static const std::array<RatchetType, 2> all_ratchet_types;
};

struct KeyScheduleEpoch
{
private:
  CipherSuite suite;

public:
  bytes joiner_secret;
  bytes epoch_secret;

  bytes sender_data_secret;
  bytes encryption_secret;
  bytes exporter_secret;
  bytes epoch_authenticator;
  bytes external_secret;
  bytes confirmation_key;
  bytes membership_key;
  bytes resumption_psk;
  bytes init_secret;

  HPKEPrivateKey external_priv;

  KeyScheduleEpoch() = default;

  // Full initializer, used by invited joiner
  static KeyScheduleEpoch joiner(CipherSuite suite_in,
                                 const bytes& joiner_secret,
                                 const std::vector<PSKWithSecret>& psks,
                                 const bytes& context);

  // Ciphersuite-only initializer, used by external joiner
  KeyScheduleEpoch(CipherSuite suite_in);

  // Initial epoch
  KeyScheduleEpoch(CipherSuite suite_in,
                   const bytes& init_secret,
                   const bytes& context);

  static std::tuple<bytes, bytes> external_init(
    CipherSuite suite,
    const HPKEPublicKey& external_pub);
  bytes receive_external_init(const bytes& kem_output) const;

  KeyScheduleEpoch next(const bytes& commit_secret,
                        const std::vector<PSKWithSecret>& psks,
                        const std::optional<bytes>& force_init_secret,
                        const bytes& context) const;

  GroupKeySource encryption_keys(LeafCount size) const;
  bytes confirmation_tag(const bytes& confirmed_transcript_hash) const;
  bytes do_export(const std::string& label,
                  const bytes& context,
                  size_t size) const;
  PSKWithSecret resumption_psk_w_secret(ResumptionPSKUsage usage,
                                        const bytes& group_id,
                                        epoch_t epoch);

  static bytes make_psk_secret(CipherSuite suite,
                               const std::vector<PSKWithSecret>& psks);
  static bytes welcome_secret(CipherSuite suite,
                              const bytes& joiner_secret,
                              const std::vector<PSKWithSecret>& psks);
  static KeyAndNonce sender_data_keys(CipherSuite suite,
                                      const bytes& sender_data_secret,
                                      const bytes& ciphertext);

  // TODO(RLB) make these methods private, but accessible to test vectors
  KeyScheduleEpoch(CipherSuite suite_in,
                   const bytes& init_secret,
                   const bytes& commit_secret,
                   const bytes& psk_secret,
                   const bytes& context);
  KeyScheduleEpoch next_raw(const bytes& commit_secret,
                            const bytes& psk_secret,
                            const std::optional<bytes>& force_init_secret,
                            const bytes& context) const;
  static bytes welcome_secret_raw(CipherSuite suite,
                                  const bytes& joiner_secret,
                                  const bytes& psk_secret);

private:
  KeyScheduleEpoch(CipherSuite suite_in,
                   const bytes& joiner_secret,
                   const bytes& psk_secret,
                   const bytes& context);
};

bool
operator==(const KeyScheduleEpoch& lhs, const KeyScheduleEpoch& rhs);

struct TranscriptHash
{
  CipherSuite suite;
  bytes confirmed;
  bytes interim;

  // For a new group
  TranscriptHash(CipherSuite suite_in);

  // For joining a group
  TranscriptHash(CipherSuite suite_in,
                 bytes confirmed_in,
                 const bytes& confirmation_tag);

  void update(const AuthenticatedContent& content_auth);
  void update_confirmed(const AuthenticatedContent& content_auth);
  void update_interim(const bytes& confirmation_tag);
  void update_interim(const AuthenticatedContent& content_auth);
};

bool
operator==(const TranscriptHash& lhs, const TranscriptHash& rhs);

} // namespace mlspp
