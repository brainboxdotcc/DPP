#include <mls/key_schedule.h>

namespace mlspp {

///
/// Key Derivation Functions
///

struct TreeContext
{
  NodeIndex node;
  uint32_t generation = 0;

  TLS_SERIALIZABLE(node, generation)
};

///
/// HashRatchet
///

HashRatchet::HashRatchet(CipherSuite suite_in, bytes base_secret_in)
  : suite(suite_in)
  , next_secret(std::move(base_secret_in))
  , next_generation(0)
  , key_size(suite.hpke().aead.key_size)
  , nonce_size(suite.hpke().aead.nonce_size)
  , secret_size(suite.secret_size())
{
}

std::tuple<uint32_t, KeyAndNonce>
HashRatchet::next()
{
  auto generation = next_generation;
  auto key = suite.derive_tree_secret(next_secret, "key", generation, key_size);
  auto nonce =
    suite.derive_tree_secret(next_secret, "nonce", generation, nonce_size);
  auto secret =
    suite.derive_tree_secret(next_secret, "secret", generation, secret_size);

  next_generation += 1;
  next_secret = secret;

  cache[generation] = { key, nonce };
  return { generation, cache.at(generation) };
}

// Note: This construction deliberately does not preserve the forward-secrecy
// invariant, in that keys/nonces are not deleted after they are used.
// Otherwise, it would not be possible for a node to send to itself.  Keys can
// be deleted once they are not needed by calling HashRatchet::erase().
KeyAndNonce
HashRatchet::get(uint32_t generation)
{
  if (cache.count(generation) > 0) {
    auto out = cache.at(generation);
    return out;
  }

  if (next_generation > generation) {
    throw ProtocolError("Request for expired key");
  }

  while (next_generation <= generation) {
    next();
  }

  return cache.at(generation);
}

void
HashRatchet::erase(uint32_t generation)
{
  if (cache.count(generation) == 0) {
    return;
  }

  cache.erase(generation);
}

///
/// SecretTree
///

SecretTree::SecretTree(CipherSuite suite_in,
                       LeafCount group_size_in,
                       bytes encryption_secret_in)
  : suite(suite_in)
  , group_size(LeafCount::full(group_size_in))
  , root(NodeIndex::root(group_size))
  , secret_size(suite_in.secret_size())
{
  secrets.emplace(root, std::move(encryption_secret_in));
}

bytes
SecretTree::get(LeafIndex sender)
{
  static const auto context_left = from_ascii("left");
  static const auto context_right = from_ascii("right");
  auto node = NodeIndex(sender);

  // Find an ancestor that is populated
  auto dirpath = node.dirpath(group_size);
  dirpath.insert(dirpath.begin(), node);
  dirpath.push_back(root);
  uint32_t curr = 0;
  for (; curr < dirpath.size(); ++curr) {
    auto i = dirpath.at(curr);
    if (secrets.count(i) > 0) {
      break;
    }
  }

  if (curr > dirpath.size()) {
    throw InvalidParameterError("No secret found to derive base key");
  }

  // Derive down
  for (; curr > 0; --curr) {
    auto curr_node = dirpath.at(curr);
    auto left = curr_node.left();
    auto right = curr_node.right();

    auto& secret = secrets.at(curr_node);

    const auto left_secret =
      suite.expand_with_label(secret, "tree", context_left, secret_size);
    const auto right_secret =
      suite.expand_with_label(secret, "tree", context_right, secret_size);

    secrets.insert_or_assign(left, left_secret);
    secrets.insert_or_assign(right, right_secret);
  }

  // Copy the leaf
  auto out = secrets.at(node);

  // Zeroize along the direct path
  for (auto i : dirpath) {
    secrets.erase(i);
  }

  return out;
}

///
/// ReuseGuard
///

static ReuseGuard
new_reuse_guard()
{
  auto random = random_bytes(4);
  auto guard = ReuseGuard();
  std::copy(random.begin(), random.end(), guard.begin());
  return guard;
}

static void
apply_reuse_guard(const ReuseGuard& guard, bytes& nonce)
{
  for (size_t i = 0; i < guard.size(); i++) {
    nonce.at(i) ^= guard.at(i);
  }
}

///
/// GroupKeySource
///

GroupKeySource::GroupKeySource(CipherSuite suite_in,
                               LeafCount group_size,
                               bytes encryption_secret)
  : suite(suite_in)
  , secret_tree(suite, group_size, std::move(encryption_secret))
{
}

HashRatchet&
GroupKeySource::chain(ContentType type, LeafIndex sender)
{
  switch (type) {
    case ContentType::proposal:
    case ContentType::commit:
      return chain(RatchetType::handshake, sender);

    case ContentType::application:
      return chain(RatchetType::application, sender);

    default:
      throw InvalidParameterError("Invalid content type");
  }
}

HashRatchet&
GroupKeySource::chain(RatchetType type, LeafIndex sender)
{
  auto key = Key{ type, sender };
  if (chains.count(key) > 0) {
    return chains[key];
  }

  auto secret_size = suite.secret_size();
  auto leaf_secret = secret_tree.get(sender);

  auto handshake_secret =
    suite.expand_with_label(leaf_secret, "handshake", {}, secret_size);
  auto application_secret =
    suite.expand_with_label(leaf_secret, "application", {}, secret_size);

  chains.emplace(Key{ RatchetType::handshake, sender },
                 HashRatchet{ suite, handshake_secret });
  chains.emplace(Key{ RatchetType::application, sender },
                 HashRatchet{ suite, application_secret });

  return chains[key];
}

std::tuple<uint32_t, ReuseGuard, KeyAndNonce>
GroupKeySource::next(ContentType type, LeafIndex sender)
{
  auto [generation, keys] = chain(type, sender).next();

  auto reuse_guard = new_reuse_guard();
  apply_reuse_guard(reuse_guard, keys.nonce);

  return { generation, reuse_guard, keys };
}

KeyAndNonce
GroupKeySource::get(ContentType type,
                    LeafIndex sender,
                    uint32_t generation,
                    ReuseGuard reuse_guard)
{
  auto keys = chain(type, sender).get(generation);
  apply_reuse_guard(reuse_guard, keys.nonce);
  return keys;
}

void
GroupKeySource::erase(ContentType type, LeafIndex sender, uint32_t generation)
{
  return chain(type, sender).erase(generation);
}

// struct {
//     opaque group_id<0..255>;
//     uint64 epoch;
//     ContentType content_type;
//     opaque authenticated_data<0..2^32-1>;
// } ContentAAD;
struct ContentAAD
{
  const bytes& group_id;
  const epoch_t epoch;
  const ContentType content_type;
  const bytes& authenticated_data;

  TLS_SERIALIZABLE(group_id, epoch, content_type, authenticated_data)
};

///
/// KeyScheduleEpoch
///

struct PSKLabel
{
  const PreSharedKeyID& id;
  uint16_t index;
  uint16_t count;

  TLS_SERIALIZABLE(id, index, count);
};

static bytes
make_joiner_secret(CipherSuite suite,
                   const bytes& context,
                   const bytes& init_secret,
                   const bytes& commit_secret)
{
  auto pre_joiner_secret = suite.hpke().kdf.extract(init_secret, commit_secret);
  return suite.expand_with_label(
    pre_joiner_secret, "joiner", context, suite.secret_size());
}

static bytes
make_epoch_secret(CipherSuite suite,
                  const bytes& joiner_secret,
                  const bytes& psk_secret,
                  const bytes& context)
{
  auto member_secret = suite.hpke().kdf.extract(joiner_secret, psk_secret);
  return suite.expand_with_label(
    member_secret, "epoch", context, suite.secret_size());
}

KeyScheduleEpoch
KeyScheduleEpoch::joiner(CipherSuite suite_in,
                         const bytes& joiner_secret,
                         const std::vector<PSKWithSecret>& psks,
                         const bytes& context)
{
  return { suite_in, joiner_secret, make_psk_secret(suite_in, psks), context };
}

KeyScheduleEpoch::KeyScheduleEpoch(CipherSuite suite_in,
                                   const bytes& joiner_secret,
                                   const bytes& psk_secret,
                                   const bytes& context)
  : suite(suite_in)
  , joiner_secret(joiner_secret)
  , epoch_secret(
      make_epoch_secret(suite_in, joiner_secret, psk_secret, context))
  , sender_data_secret(suite.derive_secret(epoch_secret, "sender data"))
  , encryption_secret(suite.derive_secret(epoch_secret, "encryption"))
  , exporter_secret(suite.derive_secret(epoch_secret, "exporter"))
  , epoch_authenticator(suite.derive_secret(epoch_secret, "authentication"))
  , external_secret(suite.derive_secret(epoch_secret, "external"))
  , confirmation_key(suite.derive_secret(epoch_secret, "confirm"))
  , membership_key(suite.derive_secret(epoch_secret, "membership"))
  , resumption_psk(suite.derive_secret(epoch_secret, "resumption"))
  , init_secret(suite.derive_secret(epoch_secret, "init"))
  , external_priv(HPKEPrivateKey::derive(suite, external_secret))
{
}

KeyScheduleEpoch::KeyScheduleEpoch(CipherSuite suite_in)
  : suite(suite_in)
{
}

KeyScheduleEpoch::KeyScheduleEpoch(CipherSuite suite_in,
                                   const bytes& init_secret,
                                   const bytes& context)
  : KeyScheduleEpoch(
      suite_in,
      make_joiner_secret(suite_in, context, init_secret, suite_in.zero()),
      { /* no PSKs */ },
      context)
{
}

KeyScheduleEpoch::KeyScheduleEpoch(CipherSuite suite_in,
                                   const bytes& init_secret,
                                   const bytes& commit_secret,
                                   const bytes& psk_secret,
                                   const bytes& context)
  : KeyScheduleEpoch(
      suite_in,
      make_joiner_secret(suite_in, context, init_secret, commit_secret),
      psk_secret,
      context)
{
}

std::tuple<bytes, bytes>
KeyScheduleEpoch::external_init(CipherSuite suite,
                                const HPKEPublicKey& external_pub)
{
  auto size = suite.secret_size();
  return external_pub.do_export(
    suite, {}, "MLS 1.0 external init secret", size);
}

bytes
KeyScheduleEpoch::receive_external_init(const bytes& kem_output) const
{
  auto size = suite.secret_size();
  return external_priv.do_export(
    suite, {}, kem_output, "MLS 1.0 external init secret", size);
}

KeyScheduleEpoch
KeyScheduleEpoch::next(const bytes& commit_secret,
                       const std::vector<PSKWithSecret>& psks,
                       const std::optional<bytes>& force_init_secret,
                       const bytes& context) const
{
  return next_raw(
    commit_secret, make_psk_secret(suite, psks), force_init_secret, context);
}

KeyScheduleEpoch
KeyScheduleEpoch::next_raw(const bytes& commit_secret,
                           const bytes& psk_secret,
                           const std::optional<bytes>& force_init_secret,
                           const bytes& context) const
{
  auto actual_init_secret = init_secret;
  if (force_init_secret) {
    actual_init_secret = opt::get(force_init_secret);
  }

  return { suite, actual_init_secret, commit_secret, psk_secret, context };
}

GroupKeySource
KeyScheduleEpoch::encryption_keys(LeafCount size) const
{
  return { suite, size, encryption_secret };
}

bytes
KeyScheduleEpoch::confirmation_tag(const bytes& confirmed_transcript_hash) const
{
  return suite.digest().hmac(confirmation_key, confirmed_transcript_hash);
}

bytes
KeyScheduleEpoch::do_export(const std::string& label,
                            const bytes& context,
                            size_t size) const
{
  auto secret = suite.derive_secret(exporter_secret, label);
  auto context_hash = suite.digest().hash(context);
  return suite.expand_with_label(secret, "exported", context_hash, size);
}

PSKWithSecret
KeyScheduleEpoch::resumption_psk_w_secret(ResumptionPSKUsage usage,
                                          const bytes& group_id,
                                          epoch_t epoch)
{
  auto nonce = random_bytes(suite.secret_size());
  auto psk = ResumptionPSK{ usage, group_id, epoch };
  return { { psk, nonce }, resumption_psk };
}

bytes
KeyScheduleEpoch::make_psk_secret(CipherSuite suite,
                                  const std::vector<PSKWithSecret>& psks)
{
  auto psk_secret = suite.zero();
  auto count = uint16_t(psks.size());
  auto index = uint16_t(0);
  for (const auto& psk : psks) {
    auto psk_extracted = suite.hpke().kdf.extract(suite.zero(), psk.secret);
    auto psk_label = tls::marshal(PSKLabel{ psk.id, index, count });
    auto psk_input = suite.expand_with_label(
      psk_extracted, "derived psk", psk_label, suite.secret_size());
    psk_secret = suite.hpke().kdf.extract(psk_input, psk_secret);
    index += 1;
  }
  return psk_secret;
}

bytes
KeyScheduleEpoch::welcome_secret(CipherSuite suite,
                                 const bytes& joiner_secret,
                                 const std::vector<PSKWithSecret>& psks)
{
  auto psk_secret = make_psk_secret(suite, psks);
  return welcome_secret_raw(suite, joiner_secret, psk_secret);
}

bytes
KeyScheduleEpoch::welcome_secret_raw(CipherSuite suite,
                                     const bytes& joiner_secret,
                                     const bytes& psk_secret)
{
  auto extract = suite.hpke().kdf.extract(joiner_secret, psk_secret);
  return suite.derive_secret(extract, "welcome");
}

KeyAndNonce
KeyScheduleEpoch::sender_data_keys(CipherSuite suite,
                                   const bytes& sender_data_secret,
                                   const bytes& ciphertext)
{
  auto sample_size = suite.secret_size();
  auto sample = bytes(sample_size);
  if (ciphertext.size() <= sample_size) {
    sample = ciphertext;
  } else {
    sample = ciphertext.slice(0, sample_size);
  }

  auto key_size = suite.hpke().aead.key_size;
  auto nonce_size = suite.hpke().aead.nonce_size;
  return {
    suite.expand_with_label(sender_data_secret, "key", sample, key_size),
    suite.expand_with_label(sender_data_secret, "nonce", sample, nonce_size),
  };
}

bool
operator==(const KeyScheduleEpoch& lhs, const KeyScheduleEpoch& rhs)
{
  auto epoch_secret = (lhs.epoch_secret == rhs.epoch_secret);
  auto sender_data_secret = (lhs.sender_data_secret == rhs.sender_data_secret);
  auto encryption_secret = (lhs.encryption_secret == rhs.encryption_secret);
  auto exporter_secret = (lhs.exporter_secret == rhs.exporter_secret);
  auto confirmation_key = (lhs.confirmation_key == rhs.confirmation_key);
  auto init_secret = (lhs.init_secret == rhs.init_secret);
  auto external_priv = (lhs.external_priv == rhs.external_priv);

  return epoch_secret && sender_data_secret && encryption_secret &&
         exporter_secret && confirmation_key && init_secret && external_priv;
}

// struct {
//     WireFormat wire_format;
//     GroupContent content; // with content.content_type == commit
//     opaque signature<V>;
// } ConfirmedTranscriptHashInput;
struct ConfirmedTranscriptHashInput
{
  WireFormat wire_format;
  const GroupContent& content;
  const bytes& signature;

  TLS_SERIALIZABLE(wire_format, content, signature)
};

// struct {
//     MAC confirmation_tag;
// } InterimTranscriptHashInput;
struct InterimTranscriptHashInput
{
  bytes confirmation_tag;

  TLS_SERIALIZABLE(confirmation_tag)
};

TranscriptHash::TranscriptHash(CipherSuite suite_in)
  : suite(suite_in)
{
}

TranscriptHash::TranscriptHash(CipherSuite suite_in,
                               bytes confirmed_in,
                               const bytes& confirmation_tag)
  : suite(suite_in)
  , confirmed(std::move(confirmed_in))
{
  update_interim(confirmation_tag);
}

void
TranscriptHash::update(const AuthenticatedContent& content_auth)
{
  update_confirmed(content_auth);
  update_interim(content_auth);
}

void
TranscriptHash::update_confirmed(const AuthenticatedContent& content_auth)
{
  const auto transcript =
    interim + content_auth.confirmed_transcript_hash_input();
  confirmed = suite.digest().hash(transcript);
}

void
TranscriptHash::update_interim(const bytes& confirmation_tag)
{
  const auto transcript = confirmed + tls::marshal(confirmation_tag);
  interim = suite.digest().hash(transcript);
}

void
TranscriptHash::update_interim(const AuthenticatedContent& content_auth)
{
  const auto transcript =
    confirmed + content_auth.interim_transcript_hash_input();
  interim = suite.digest().hash(transcript);
}

bool
operator==(const TranscriptHash& lhs, const TranscriptHash& rhs)
{
  auto confirmed = (lhs.confirmed == rhs.confirmed);
  auto interim = (lhs.interim == rhs.interim);
  return confirmed && interim;
}

} // namespace mlspp
