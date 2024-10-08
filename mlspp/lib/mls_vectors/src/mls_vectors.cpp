#include <mls/key_schedule.h>
#include <mls/state.h>
#include <mls/tree_math.h>
#include <mls_vectors/mls_vectors.h>

#include <iostream> // XXX

namespace mls_vectors {

using namespace mlspp;

///
/// Assertions for verifying test vectors
///

template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
std::ostream&
operator<<(std::ostream& str, const T& obj)
{
  auto u = static_cast<std::underlying_type_t<T>>(obj);
  return str << u;
}

static std::ostream&
operator<<(std::ostream& str, const NodeIndex& obj)
{
  return str << obj.val;
}

static std::ostream&
operator<<(std::ostream& str, const NodeCount& obj)
{
  return str << obj.val;
}

template<typename T>
static std::ostream&
operator<<(std::ostream& str, const std::optional<T>& obj)
{
  if (!obj) {
    return str << "(nullopt)";
  }

  return str << opt::get(obj);
}

static std::ostream&
operator<<(std::ostream& str, const std::vector<uint8_t>& obj)
{
  return str << to_hex(obj);
}

template<typename T>
static std::ostream&
operator<<(std::ostream& str, const std::vector<T>& obj)
{
  for (const auto& val : obj) {
    str << val << " ";
  }
  return str;
}

static std::ostream&
operator<<(std::ostream& str, const GroupContent::RawContent& obj)
{
  return var::visit(
    overloaded{
      [&](const Proposal&) -> std::ostream& { return str << "[Proposal]"; },
      [&](const Commit&) -> std::ostream& { return str << "[Commit]"; },
      [&](const ApplicationData&) -> std::ostream& {
        return str << "[ApplicationData]";
      },
    },
    obj);
}

template<typename T>
inline std::enable_if_t<T::_tls_serializable, std::ostream&>
operator<<(std::ostream& str, const T& obj)
{
  return str << to_hex(tls::marshal(obj));
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VERIFY(label, test)                                                    \
  if (auto err = verify_bool(label, test)) {                                   \
    return err;                                                                \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VERIFY_EQUAL(label, actual, expected)                                  \
  if (auto err = verify_equal(label, actual, expected)) {                      \
    return err;                                                                \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VERIFY_TLS_RTT(label, Type, expected)                                  \
  if (auto err = verify_round_trip<Type>(label, expected)) {                   \
    return err;                                                                \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VERIFY_TLS_RTT_VAL(label, Type, expected, val)                         \
  if (auto err = verify_round_trip<Type>(label, expected, val)) {              \
    return err;                                                                \
  }

template<typename T>
static std::optional<std::string>
verify_bool(const std::string& label, const T& test)
{
  if (test) {
    return std::nullopt;
  }

  return label;
}

template<typename T, typename U>
static std::optional<std::string>
verify_equal(const std::string& label, const T& actual, const U& expected)
{
  if (actual == expected) {
    return std::nullopt;
  }

  auto ss = std::stringstream();
  ss << "Error: " << label << "  " << actual << " != " << expected;
  return ss.str();
}

template<typename T>
static std::optional<std::string>
verify_round_trip(const std::string& label, const bytes& expected)
{
  auto noop = [](const auto& /* unused */) { return true; };
  return verify_round_trip<T>(label, expected, noop);
}

template<typename T, typename F>
static std::optional<std::string>
verify_round_trip(const std::string& label, const bytes& expected, const F& val)
{
  auto obj = T{};
  try {
    obj = tls::get<T>(expected);
  } catch (const std::exception& e) {
    auto ss = std::stringstream();
    ss << "Decode error: " << label << " " << e.what();
    return ss.str();
  }

  if (!val(obj)) {
    auto ss = std::stringstream();
    ss << "Validation error: " << label;
    return ss.str();
  }

  auto actual = tls::marshal(obj);
  VERIFY_EQUAL(label, actual, expected);
  return std::nullopt;
}

///
/// PseudoRandom
///

PseudoRandom::Generator::Generator(CipherSuite suite_in,
                                   const std::string& label)
  : suite(suite_in)
  , seed(suite.hpke().kdf.extract({}, from_ascii(label)))
{
}

PseudoRandom::Generator::Generator(CipherSuite suite_in, bytes seed_in)
  : suite(suite_in)
  , seed(std::move(seed_in))
{
}

PseudoRandom::Generator
PseudoRandom::Generator::sub(const std::string& label) const
{
  return { suite, suite.derive_secret(seed, label) };
}

bytes
PseudoRandom::Generator::secret(const std::string& label) const
{
  return suite.derive_secret(seed, label);
}

bytes
PseudoRandom::Generator::generate(const std::string& label, size_t size) const
{
  return suite.expand_with_label(seed, label, {}, size);
}

uint16_t
PseudoRandom::Generator::uint16(const std::string& label) const
{
  auto data = generate(label, 2);
  return tls::get<uint16_t>(data);
}

uint32_t
PseudoRandom::Generator::uint32(const std::string& label) const
{
  auto data = generate(label, 4);
  return tls::get<uint16_t>(data);
}

uint64_t
PseudoRandom::Generator::uint64(const std::string& label) const
{
  auto data = generate(label, 8);
  return tls::get<uint16_t>(data);
}

SignaturePrivateKey
PseudoRandom::Generator::signature_key(const std::string& label) const
{
  auto data = generate(label, suite.secret_size());
  return SignaturePrivateKey::derive(suite, data);
}

HPKEPrivateKey
PseudoRandom::Generator::hpke_key(const std::string& label) const
{
  auto data = generate(label, suite.secret_size());
  return HPKEPrivateKey::derive(suite, data);
}

size_t
PseudoRandom::Generator::output_length() const
{
  return suite.secret_size();
}

PseudoRandom::PseudoRandom(CipherSuite suite, const std::string& label)
  : prg(suite, label)
{
}

///
/// TreeMathTestVector
///

// XXX(RLB): This is a hack to get the tests working in the right format.  In
// reality, the tree math functions should be updated to be fallible.
std::optional<mlspp::NodeIndex>
TreeMathTestVector::null_if_invalid(NodeIndex input, NodeIndex answer) const
{
  // For some invalid cases (e.g., leaf.left()), we currently return the node
  // itself instead of null
  if (input == answer) {
    return std::nullopt;
  }

  // NodeIndex::parent is irrespective of tree size, so we might step out of the
  // tree under consideration.
  if (answer.val >= n_nodes.val) {
    return std::nullopt;
  }

  return answer;
}

TreeMathTestVector::TreeMathTestVector(uint32_t n_leaves_in)
  : n_leaves(n_leaves_in)
  , n_nodes(n_leaves)
  , root(NodeIndex::root(n_leaves))
  , left(n_nodes.val)
  , right(n_nodes.val)
  , parent(n_nodes.val)
  , sibling(n_nodes.val)
{
  for (NodeIndex x{ 0 }; x.val < n_nodes.val; x.val++) {
    left[x.val] = null_if_invalid(x, x.left());
    right[x.val] = null_if_invalid(x, x.right());
    parent[x.val] = null_if_invalid(x, x.parent());
    sibling[x.val] = null_if_invalid(x, x.sibling());
  }
}

std::optional<std::string>
TreeMathTestVector::verify() const
{
  VERIFY_EQUAL("n_nodes", n_nodes, NodeCount(n_leaves));
  VERIFY_EQUAL("root", root, NodeIndex::root(n_leaves));

  for (NodeIndex x{ 0 }; x.val < n_nodes.val; x.val++) {
    VERIFY_EQUAL("left", null_if_invalid(x, x.left()), left[x.val]);
    VERIFY_EQUAL("right", null_if_invalid(x, x.right()), right[x.val]);
    VERIFY_EQUAL("parent", null_if_invalid(x, x.parent()), parent[x.val]);
    VERIFY_EQUAL("sibling", null_if_invalid(x, x.sibling()), sibling[x.val]);
  }

  return std::nullopt;
}

///
/// TreeMathTestVector
///

CryptoBasicsTestVector::RefHash::RefHash(CipherSuite suite,
                                         const PseudoRandom::Generator& prg)
  : label("RefHash")
  , value(prg.secret("value"))
  , out(suite.raw_ref(from_ascii(label), value))
{
}

std::optional<std::string>
CryptoBasicsTestVector::RefHash::verify(CipherSuite suite) const
{
  VERIFY_EQUAL("ref hash", out, suite.raw_ref(from_ascii(label), value));
  return std::nullopt;
}

CryptoBasicsTestVector::ExpandWithLabel::ExpandWithLabel(
  CipherSuite suite,
  const PseudoRandom::Generator& prg)
  : secret(prg.secret("secret"))
  , label("ExpandWithLabel")
  , context(prg.secret("context"))
  , length(static_cast<uint16_t>(prg.output_length()))
  , out(suite.expand_with_label(secret, label, context, length))
{
}

std::optional<std::string>
CryptoBasicsTestVector::ExpandWithLabel::verify(CipherSuite suite) const
{
  VERIFY_EQUAL("expand with label",
               out,
               suite.expand_with_label(secret, label, context, length));
  return std::nullopt;
}

CryptoBasicsTestVector::DeriveSecret::DeriveSecret(
  CipherSuite suite,
  const PseudoRandom::Generator& prg)
  : secret(prg.secret("secret"))
  , label("DeriveSecret")
  , out(suite.derive_secret(secret, label))
{
}

std::optional<std::string>
CryptoBasicsTestVector::DeriveSecret::verify(CipherSuite suite) const
{
  VERIFY_EQUAL("derive secret", out, suite.derive_secret(secret, label));
  return std::nullopt;
}

CryptoBasicsTestVector::DeriveTreeSecret::DeriveTreeSecret(
  CipherSuite suite,
  const PseudoRandom::Generator& prg)
  : secret(prg.secret("secret"))
  , label("DeriveTreeSecret")
  , generation(prg.uint32("generation"))
  , length(static_cast<uint16_t>(prg.output_length()))
  , out(suite.derive_tree_secret(secret, label, generation, length))
{
}

std::optional<std::string>
CryptoBasicsTestVector::DeriveTreeSecret::verify(CipherSuite suite) const
{
  VERIFY_EQUAL("derive tree secret",
               out,
               suite.derive_tree_secret(secret, label, generation, length));
  return std::nullopt;
}

CryptoBasicsTestVector::SignWithLabel::SignWithLabel(
  CipherSuite suite,
  const PseudoRandom::Generator& prg)
  : priv(prg.signature_key("priv"))
  , pub(priv.public_key)
  , content(prg.secret("content"))
  , label("SignWithLabel")
  , signature(priv.sign(suite, label, content))
{
}

std::optional<std::string>
CryptoBasicsTestVector::SignWithLabel::verify(CipherSuite suite) const
{
  VERIFY("verify with label", pub.verify(suite, label, content, signature));

  auto new_signature = priv.sign(suite, label, content);
  VERIFY("sign with label", pub.verify(suite, label, content, new_signature));

  return std::nullopt;
}

CryptoBasicsTestVector::EncryptWithLabel::EncryptWithLabel(
  CipherSuite suite,
  const PseudoRandom::Generator& prg)
  : priv(prg.hpke_key("priv"))
  , pub(priv.public_key)
  , label("EncryptWithLabel")
  , context(prg.secret("context"))
  , plaintext(prg.secret("plaintext"))
{
  auto ct = pub.encrypt(suite, label, context, plaintext);
  kem_output = ct.kem_output;
  ciphertext = ct.ciphertext;
}

std::optional<std::string>
CryptoBasicsTestVector::EncryptWithLabel::verify(CipherSuite suite) const
{
  auto ct = HPKECiphertext{ kem_output, ciphertext };
  auto pt = priv.decrypt(suite, label, context, ct);
  VERIFY_EQUAL("decrypt with label", pt, plaintext);

  auto new_ct = pub.encrypt(suite, label, context, plaintext);
  auto new_pt = priv.decrypt(suite, label, context, new_ct);
  VERIFY_EQUAL("encrypt with label", new_pt, plaintext);

  return std::nullopt;
}

CryptoBasicsTestVector::CryptoBasicsTestVector(CipherSuite suite)
  : PseudoRandom(suite, "crypto-basics")
  , cipher_suite(suite)
  , ref_hash(suite, prg.sub("ref_hash"))
  , expand_with_label(suite, prg.sub("expand_with_label"))
  , derive_secret(suite, prg.sub("derive_secret"))
  , derive_tree_secret(suite, prg.sub("derive_tree_secret"))
  , sign_with_label(suite, prg.sub("sign_with_label"))
  , encrypt_with_label(suite, prg.sub("encrypt_with_label"))
{
}

std::optional<std::string>
CryptoBasicsTestVector::verify() const
{
  auto result = ref_hash.verify(cipher_suite);
  if (result) {
    return result;
  }

  result = expand_with_label.verify(cipher_suite);
  if (result) {
    return result;
  }

  result = derive_secret.verify(cipher_suite);
  if (result) {
    return result;
  }

  result = derive_tree_secret.verify(cipher_suite);
  if (result) {
    return result;
  }

  result = sign_with_label.verify(cipher_suite);
  if (result) {
    return result;
  }

  result = encrypt_with_label.verify(cipher_suite);
  if (result) {
    return result;
  }

  return std::nullopt;
}

///
/// SecretTreeTestVector
///

SecretTreeTestVector::SenderData::SenderData(mlspp::CipherSuite suite,
                                             const PseudoRandom::Generator& prg)
  : sender_data_secret(prg.secret("sender_data_secret"))
  , ciphertext(prg.secret("ciphertext"))
{
  auto key_and_nonce =
    KeyScheduleEpoch::sender_data_keys(suite, sender_data_secret, ciphertext);
  key = key_and_nonce.key;
  nonce = key_and_nonce.nonce;
}

std::optional<std::string>
SecretTreeTestVector::SenderData::verify(mlspp::CipherSuite suite) const
{
  auto key_and_nonce =
    KeyScheduleEpoch::sender_data_keys(suite, sender_data_secret, ciphertext);
  VERIFY_EQUAL("sender data key", key, key_and_nonce.key);
  VERIFY_EQUAL("sender data nonce", nonce, key_and_nonce.nonce);
  return std::nullopt;
}

SecretTreeTestVector::SecretTreeTestVector(
  mlspp::CipherSuite suite,
  uint32_t n_leaves,
  const std::vector<uint32_t>& generations)
  : PseudoRandom(suite, "secret-tree")
  , cipher_suite(suite)
  , sender_data(suite, prg.sub("sender_data"))
  , encryption_secret(prg.secret("encryption_secret"))
{
  auto src =
    GroupKeySource(cipher_suite, LeafCount{ n_leaves }, encryption_secret);
  leaves.resize(n_leaves);
  auto zero_reuse_guard = ReuseGuard{ 0, 0, 0, 0 };
  for (uint32_t i = 0; i < n_leaves; i++) {
    auto leaf = LeafIndex{ i };

    for (const auto generation : generations) {
      auto hs =
        src.get(ContentType::proposal, leaf, generation, zero_reuse_guard);
      auto app =
        src.get(ContentType::application, leaf, generation, zero_reuse_guard);

      leaves.at(i).push_back(
        RatchetStep{ generation, hs.key, hs.nonce, app.key, app.nonce });

      src.erase(ContentType::proposal, leaf, generation);
      src.erase(ContentType::application, leaf, generation);
    }
  }
}

std::optional<std::string>
SecretTreeTestVector::verify() const
{
  auto sender_data_error = sender_data.verify(cipher_suite);
  if (sender_data_error) {
    return sender_data_error;
  }

  auto n_leaves = static_cast<uint32_t>(leaves.size());
  auto src =
    GroupKeySource(cipher_suite, LeafCount{ n_leaves }, encryption_secret);
  auto zero_reuse_guard = ReuseGuard{ 0, 0, 0, 0 };
  for (uint32_t i = 0; i < n_leaves; i++) {
    auto leaf = LeafIndex{ i };

    for (const auto& step : leaves[i]) {
      auto generation = step.generation;

      auto hs =
        src.get(ContentType::proposal, leaf, generation, zero_reuse_guard);
      VERIFY_EQUAL("hs key", hs.key, step.handshake_key);
      VERIFY_EQUAL("hs nonce", hs.nonce, step.handshake_nonce);

      auto app =
        src.get(ContentType::application, leaf, generation, zero_reuse_guard);
      VERIFY_EQUAL("app key", app.key, step.application_key);
      VERIFY_EQUAL("app nonce", app.nonce, step.application_nonce);
    }
  }

  return std::nullopt;
}

///
/// KeyScheduleTestVector
///

KeyScheduleTestVector::KeyScheduleTestVector(CipherSuite suite,
                                             uint32_t n_epochs)
  : PseudoRandom(suite, "key-schedule")
  , cipher_suite(suite)
  , group_id(prg.secret("group_id"))
  , initial_init_secret(prg.secret("group_id"))
{
  auto group_context = GroupContext{ suite, group_id, 0, {}, {}, {} };
  auto epoch = KeyScheduleEpoch(cipher_suite);
  epoch.init_secret = initial_init_secret;

  for (uint64_t i = 0; i < n_epochs; i++) {
    auto epoch_prg = prg.sub(to_hex(tls::marshal(i)));

    group_context.tree_hash = epoch_prg.secret("tree_hash");
    group_context.confirmed_transcript_hash =
      epoch_prg.secret("confirmed_transcript_hash");
    auto ctx = tls::marshal(group_context);

    // TODO(RLB) Add Test case for externally-driven epoch change
    auto commit_secret = epoch_prg.secret("commit_secret");
    auto psk_secret = epoch_prg.secret("psk_secret");
    epoch = epoch.next_raw(commit_secret, psk_secret, std::nullopt, ctx);

    auto welcome_secret = KeyScheduleEpoch::welcome_secret_raw(
      cipher_suite, epoch.joiner_secret, psk_secret);

    auto exporter_prg = epoch_prg.sub("exporter");
    auto exporter_label = to_hex(exporter_prg.secret("label"));
    auto exporter_context = exporter_prg.secret("context");
    auto exporter_length = cipher_suite.secret_size();
    auto exported =
      epoch.do_export(exporter_label, exporter_context, exporter_length);

    epochs.push_back({ group_context.tree_hash,
                       commit_secret,
                       psk_secret,
                       group_context.confirmed_transcript_hash,

                       ctx,

                       epoch.joiner_secret,
                       welcome_secret,
                       epoch.init_secret,

                       epoch.sender_data_secret,
                       epoch.encryption_secret,
                       epoch.exporter_secret,
                       epoch.epoch_authenticator,
                       epoch.external_secret,
                       epoch.confirmation_key,
                       epoch.membership_key,
                       epoch.resumption_psk,

                       epoch.external_priv.public_key,

                       {
                         exporter_label,
                         exporter_context,
                         exporter_length,
                         exported,
                       } });

    group_context.epoch += 1;
  }
}

std::optional<std::string>
KeyScheduleTestVector::verify() const
{
  auto group_context = GroupContext{ cipher_suite, group_id, 0, {}, {}, {} };
  auto epoch = KeyScheduleEpoch(cipher_suite);
  epoch.init_secret = initial_init_secret;

  for (const auto& tve : epochs) {
    group_context.tree_hash = tve.tree_hash;
    group_context.confirmed_transcript_hash = tve.confirmed_transcript_hash;
    auto ctx = tls::marshal(group_context);
    VERIFY_EQUAL("group context", ctx, tve.group_context);

    epoch =
      epoch.next_raw(tve.commit_secret, tve.psk_secret, std::nullopt, ctx);

    // Verify the rest of the epoch
    VERIFY_EQUAL("joiner secret", epoch.joiner_secret, tve.joiner_secret);

    auto welcome_secret = KeyScheduleEpoch::welcome_secret_raw(
      cipher_suite, tve.joiner_secret, tve.psk_secret);
    VERIFY_EQUAL("welcome secret", welcome_secret, tve.welcome_secret);

    VERIFY_EQUAL(
      "sender data secret", epoch.sender_data_secret, tve.sender_data_secret);
    VERIFY_EQUAL(
      "encryption secret", epoch.encryption_secret, tve.encryption_secret);
    VERIFY_EQUAL("exporter secret", epoch.exporter_secret, tve.exporter_secret);
    VERIFY_EQUAL("epoch authenticator",
                 epoch.epoch_authenticator,
                 tve.epoch_authenticator);
    VERIFY_EQUAL("external secret", epoch.external_secret, tve.external_secret);
    VERIFY_EQUAL(
      "confirmation key", epoch.confirmation_key, tve.confirmation_key);
    VERIFY_EQUAL("membership key", epoch.membership_key, tve.membership_key);
    VERIFY_EQUAL("resumption psk", epoch.resumption_psk, tve.resumption_psk);
    VERIFY_EQUAL("init secret", epoch.init_secret, tve.init_secret);

    VERIFY_EQUAL(
      "external pub", epoch.external_priv.public_key, tve.external_pub);

    auto exported = epoch.do_export(
      tve.exporter.label, tve.exporter.context, tve.exporter.length);
    VERIFY_EQUAL("exported", exported, tve.exporter.secret);

    group_context.epoch += 1;
  }

  return std::nullopt;
}

///
/// MessageProtectionTestVector
///

MessageProtectionTestVector::MessageProtectionTestVector(CipherSuite suite)
  : PseudoRandom(suite, "message-protection")
  , cipher_suite(suite)
  , group_id(prg.secret("group_id"))
  , epoch(prg.uint64("epoch"))
  , tree_hash(prg.secret("tree_hash"))
  , confirmed_transcript_hash(prg.secret("confirmed_transcript_hash"))
  , signature_priv(prg.signature_key("signature_priv"))
  , signature_pub(signature_priv.public_key)
  , encryption_secret(prg.secret("encryption_secret"))
  , sender_data_secret(prg.secret("sender_data_secret"))
  , membership_key(prg.secret("membership_key"))
  , proposal{ GroupContextExtensions{} }
  , commit{ /* XXX(RLB) this is technically invalid, empty w/o path */ }
  , application{ prg.secret("application") }
{
  proposal_pub = protect_pub(proposal);
  proposal_priv = protect_priv(proposal);

  commit_pub = protect_pub(commit);
  commit_priv = protect_priv(commit);

  application_priv = protect_priv(ApplicationData{ application });
}

std::optional<std::string>
MessageProtectionTestVector::verify()
{
  // Initialize fields that don't get set from JSON
  prg = PseudoRandom::Generator(cipher_suite, "message-protection");
  signature_priv.set_public_key(cipher_suite);

  // Sanity check the key pairs
  VERIFY_EQUAL("sig kp", signature_priv.public_key, signature_pub);

  // Verify proposal unprotect as PublicMessage
  auto proposal_pub_unprotected = unprotect(proposal_pub);
  VERIFY("proposal pub unprotect auth", proposal_pub_unprotected);
  VERIFY_EQUAL("proposal pub unprotect",
               opt::get(proposal_pub_unprotected).content,
               proposal);

  // Verify proposal unprotect as PrivateMessage
  auto proposal_priv_unprotected = unprotect(proposal_priv);
  VERIFY("proposal priv unprotect auth", proposal_priv_unprotected);
  VERIFY_EQUAL("proposal priv unprotect",
               opt::get(proposal_priv_unprotected).content,
               proposal);

  // Verify commit unprotect as PublicMessage
  auto commit_pub_unprotected = unprotect(commit_pub);
  VERIFY("commit pub unprotect auth", commit_pub_unprotected);
  VERIFY_EQUAL(
    "commit pub unprotect", opt::get(commit_pub_unprotected).content, commit);

  // Verify commit unprotect as PrivateMessage
  auto commit_priv_unprotected = unprotect(commit_priv);
  VERIFY("commit priv unprotect auth", commit_priv_unprotected);
  VERIFY_EQUAL(
    "commit priv unprotect", opt::get(commit_priv_unprotected).content, commit);

  // Verify application data unprotect as PrivateMessage
  auto app_unprotected = unprotect(application_priv);
  VERIFY("app priv unprotect auth", app_unprotected);
  VERIFY_EQUAL("app priv unprotect",
               opt::get(app_unprotected).content,
               ApplicationData{ application });

  // Verify protect/unprotect round-trips
  // XXX(RLB): Note that because (a) unprotect() deletes keys from the ratchet
  // and (b) we are using the same ratchet to send and receive, we need to do
  // these round-trip tests after all the unprotect tests are done.  Otherwise
  // the protect() calls here will re-use generations used the test vector, and
  // then unprotect() will delete the keys, then when you go to decrypt the test
  // vector object, you'll get "expired key".  It might be good to have better
  // safeguards around such reuse.
  auto proposal_pub_protected = protect_pub(proposal);
  auto proposal_pub_protected_unprotected = unprotect(proposal_pub_protected);
  VERIFY("proposal pub protect/unprotect auth",
         proposal_pub_protected_unprotected);
  VERIFY_EQUAL("proposal pub protect/unprotect",
               opt::get(proposal_pub_protected_unprotected).content,
               proposal);

  auto proposal_priv_protected = protect_priv(proposal);
  auto proposal_priv_protected_unprotected = unprotect(proposal_priv_protected);
  VERIFY("proposal priv protect/unprotect auth",
         proposal_priv_protected_unprotected);
  VERIFY_EQUAL("proposal priv protect/unprotect",
               opt::get(proposal_priv_protected_unprotected).content,
               proposal);

  auto commit_pub_protected = protect_pub(commit);
  auto commit_pub_protected_unprotected = unprotect(commit_pub_protected);
  VERIFY("commit pub protect/unprotect auth", commit_pub_protected_unprotected);
  VERIFY_EQUAL("commit pub protect/unprotect",
               opt::get(commit_pub_protected_unprotected).content,
               commit);

  auto commit_priv_protected = protect_priv(commit);
  auto commit_priv_protected_unprotected = unprotect(commit_priv_protected);
  VERIFY("commit priv protect/unprotect auth",
         commit_priv_protected_unprotected);
  VERIFY_EQUAL("commit priv protect/unprotect",
               opt::get(commit_priv_protected_unprotected).content,
               commit);

  auto app_protected = protect_priv(ApplicationData{ application });
  auto app_protected_unprotected = unprotect(app_protected);
  VERIFY("app priv protect/unprotect auth", app_protected_unprotected);
  VERIFY_EQUAL("app priv protect/unprotect",
               opt::get(app_protected_unprotected).content,
               ApplicationData{ application });

  return std::nullopt;
}

GroupKeySource
MessageProtectionTestVector::group_keys() const
{
  return { cipher_suite, LeafCount{ 2 }, encryption_secret };
}

GroupContext
MessageProtectionTestVector::group_context() const
{
  return GroupContext{
    cipher_suite, group_id, epoch, tree_hash, confirmed_transcript_hash, {}
  };
}

MLSMessage
MessageProtectionTestVector::protect_pub(
  const mlspp::GroupContent::RawContent& raw_content) const
{
  auto sender = Sender{ MemberSender{ LeafIndex{ 1 } } };
  auto authenticated_data = bytes{};

  auto content =
    GroupContent{ group_id, epoch, sender, authenticated_data, raw_content };

  auto auth_content = AuthenticatedContent::sign(WireFormat::mls_public_message,
                                                 content,
                                                 cipher_suite,
                                                 signature_priv,
                                                 group_context());
  if (content.content_type() == ContentType::commit) {
    auto confirmation_tag = prg.secret("confirmation_tag");
    auth_content.set_confirmation_tag(confirmation_tag);
  }

  return PublicMessage::protect(
    auth_content, cipher_suite, membership_key, group_context());
}

MLSMessage
MessageProtectionTestVector::protect_priv(
  const mlspp::GroupContent::RawContent& raw_content)
{
  auto sender = Sender{ MemberSender{ LeafIndex{ 1 } } };
  auto authenticated_data = bytes{};
  auto padding_size = size_t(0);

  auto content =
    GroupContent{ group_id, epoch, sender, authenticated_data, raw_content };

  auto auth_content =
    AuthenticatedContent::sign(WireFormat::mls_private_message,
                               content,
                               cipher_suite,
                               signature_priv,
                               group_context());
  if (content.content_type() == ContentType::commit) {
    auto confirmation_tag = prg.secret("confirmation_tag");
    auth_content.set_confirmation_tag(confirmation_tag);
  }

  auto keys = group_keys();
  return PrivateMessage::protect(
    auth_content, cipher_suite, keys, sender_data_secret, padding_size);
}

std::optional<GroupContent>
MessageProtectionTestVector::unprotect(const MLSMessage& message)
{
  auto do_unprotect =
    overloaded{ [&](const PublicMessage& pt) {
                 return pt.unprotect(
                   cipher_suite, membership_key, group_context());
               },
                [&](const PrivateMessage& ct) {
                  auto keys = group_keys();
                  return ct.unprotect(cipher_suite, keys, sender_data_secret);
                },
                [](const auto& /* other */) -> std::optional<ValidatedContent> {
                  return std::nullopt;
                } };

  auto maybe_auth_content = var::visit(do_unprotect, message.message);
  if (!maybe_auth_content) {
    return std::nullopt;
  }

  auto val_content = opt::get(maybe_auth_content);
  const auto& auth_content = val_content.authenticated_content();
  if (!auth_content.verify(cipher_suite, signature_pub, group_context())) {
    return std::nullopt;
  }

  return auth_content.content;
}

///
/// PSKTestVector
///
static std::vector<PSKWithSecret>
to_psk_w_secret(const std::vector<PSKSecretTestVector::PSK>& psks)
{
  auto pskws = std::vector<PSKWithSecret>(psks.size());
  std::transform(
    std::begin(psks), std::end(psks), std::begin(pskws), [](const auto& psk) {
      auto ext_id = ExternalPSK{ psk.psk_id };
      auto id = PreSharedKeyID{ ext_id, psk.psk_nonce };
      return PSKWithSecret{ id, psk.psk };
    });

  return pskws;
}

PSKSecretTestVector::PSKSecretTestVector(mlspp::CipherSuite suite,
                                         size_t n_psks)
  : PseudoRandom(suite, "psk_secret")
  , cipher_suite(suite)
  , psks(n_psks)
{
  uint32_t i = 0;
  for (auto& psk : psks) {
    auto ix = to_hex(tls::marshal(i));
    i += 1;

    psk.psk_id = prg.secret("psk_id" + ix);
    psk.psk_nonce = prg.secret("psk_nonce" + ix);
    psk.psk = prg.secret("psk" + ix);
  }

  psk_secret =
    KeyScheduleEpoch::make_psk_secret(cipher_suite, to_psk_w_secret(psks));
}

std::optional<std::string>
PSKSecretTestVector::verify() const
{
  auto actual =
    KeyScheduleEpoch::make_psk_secret(cipher_suite, to_psk_w_secret(psks));
  VERIFY_EQUAL("psk secret", actual, psk_secret);

  return std::nullopt;
}

///
/// TranscriptTestVector
///
TranscriptTestVector::TranscriptTestVector(CipherSuite suite)
  : PseudoRandom(suite, "transcript")
  , cipher_suite(suite)
  , interim_transcript_hash_before(prg.secret("interim_transcript_hash_before"))
{
  auto transcript = TranscriptHash(suite);
  transcript.interim = interim_transcript_hash_before;

  auto group_id = prg.secret("group_id");
  auto epoch = prg.uint64("epoch");
  auto group_context_obj =
    GroupContext{ suite,
                  group_id,
                  epoch,
                  prg.secret("tree_hash_before"),
                  prg.secret("confirmed_transcript_hash_before"),
                  {} };
  auto group_context = tls::marshal(group_context_obj);

  auto init_secret = prg.secret("init_secret");
  auto ks_epoch = KeyScheduleEpoch(suite, init_secret, group_context);

  auto sig_priv = prg.signature_key("sig_priv");
  auto leaf_index = LeafIndex{ 0 };

  authenticated_content = AuthenticatedContent::sign(
    WireFormat::mls_public_message,
    GroupContent{
      group_id, epoch, { MemberSender{ leaf_index } }, {}, Commit{} },
    suite,
    sig_priv,
    group_context_obj);

  transcript.update_confirmed(authenticated_content);

  const auto confirmation_tag = ks_epoch.confirmation_tag(transcript.confirmed);
  authenticated_content.set_confirmation_tag(confirmation_tag);

  transcript.update_interim(authenticated_content);

  // Store the required data
  confirmation_key = ks_epoch.confirmation_key;
  confirmed_transcript_hash_after = transcript.confirmed;
  interim_transcript_hash_after = transcript.interim;
}

std::optional<std::string>
TranscriptTestVector::verify() const
{
  auto transcript = TranscriptHash(cipher_suite);
  transcript.interim = interim_transcript_hash_before;

  transcript.update(authenticated_content);
  VERIFY_EQUAL(
    "confirmed", transcript.confirmed, confirmed_transcript_hash_after);
  VERIFY_EQUAL("interim", transcript.interim, interim_transcript_hash_after);

  auto confirmation_tag =
    cipher_suite.digest().hmac(confirmation_key, transcript.confirmed);
  VERIFY_EQUAL("confirmation tag",
               confirmation_tag,
               authenticated_content.auth.confirmation_tag);

  return std::nullopt;
}

///
/// WelcomeTestVector
///
WelcomeTestVector::WelcomeTestVector(CipherSuite suite)
  : PseudoRandom(suite, "welcome")
  , cipher_suite(suite)
  , init_priv(prg.hpke_key("init_priv"))
{
  auto joiner_secret = prg.secret("joiner_secret");
  auto group_id = prg.secret("group_id");
  auto epoch = epoch_t(prg.uint64("epoch"));
  auto tree_hash = prg.secret("tree_hash");
  auto confirmed_transcript_hash = prg.secret("confirmed_transcript_hash");
  auto enc_priv = prg.hpke_key("enc_priv");
  auto sig_priv = prg.signature_key("sig_priv");
  auto cred = Credential::basic(prg.secret("identity"));

  auto signer_index = LeafIndex{ prg.uint32("signer") };
  auto signer_priv = prg.signature_key("signer_priv");
  signer_pub = signer_priv.public_key;

  auto leaf_node = LeafNode{
    cipher_suite,
    enc_priv.public_key,
    sig_priv.public_key,
    cred,
    Capabilities::create_default(),
    Lifetime::create_default(),
    {},
    sig_priv,
  };
  auto key_package_obj = KeyPackage{
    cipher_suite, init_priv.public_key, leaf_node, {}, sig_priv,
  };
  key_package = key_package_obj;

  auto group_context = GroupContext{
    cipher_suite, group_id, epoch, tree_hash, confirmed_transcript_hash, {}
  };

  auto key_schedule = KeyScheduleEpoch::joiner(
    cipher_suite, joiner_secret, {}, tls::marshal(group_context));
  auto confirmation_tag =
    key_schedule.confirmation_tag(confirmed_transcript_hash);

  auto group_info = GroupInfo{
    group_context,
    {},
    confirmation_tag,
  };
  group_info.sign(signer_index, signer_priv);

  auto welcome_obj = Welcome(cipher_suite, joiner_secret, {}, group_info);
  welcome_obj.encrypt(key_package_obj, std::nullopt);
  welcome = welcome_obj;
}

std::optional<std::string>
WelcomeTestVector::verify() const
{
  VERIFY_EQUAL(
    "kp format", key_package.wire_format(), WireFormat::mls_key_package);
  VERIFY_EQUAL(
    "welcome format", welcome.wire_format(), WireFormat::mls_welcome);

  const auto& key_package_obj = var::get<KeyPackage>(key_package.message);
  const auto& welcome_obj = var::get<Welcome>(welcome.message);

  VERIFY_EQUAL("kp suite", key_package_obj.cipher_suite, cipher_suite);
  VERIFY_EQUAL("welcome suite", welcome_obj.cipher_suite, cipher_suite);

  auto maybe_kpi = welcome_obj.find(key_package_obj);
  VERIFY("found key package", maybe_kpi);

  auto kpi = opt::get(maybe_kpi);
  auto group_secrets = welcome_obj.decrypt_secrets(kpi, init_priv);
  auto group_info = welcome_obj.decrypt(group_secrets.joiner_secret, {});

  // Verify signature on GroupInfo
  VERIFY("group info verify", group_info.verify(signer_pub));

  // Verify confirmation tag
  const auto& group_context = group_info.group_context;
  auto key_schedule = KeyScheduleEpoch::joiner(
    cipher_suite, group_secrets.joiner_secret, {}, tls::marshal(group_context));
  auto confirmation_tag =
    key_schedule.confirmation_tag(group_context.confirmed_transcript_hash);

  return std::nullopt;
}

///
/// TreeTestCase
///

std::array<TreeStructure, 14> all_tree_structures{
  TreeStructure::full_tree_2,
  TreeStructure::full_tree_3,
  TreeStructure::full_tree_4,
  TreeStructure::full_tree_5,
  TreeStructure::full_tree_6,
  TreeStructure::full_tree_7,
  TreeStructure::full_tree_8,
  TreeStructure::full_tree_32,
  TreeStructure::full_tree_33,
  TreeStructure::full_tree_34,
  TreeStructure::internal_blanks_no_skipping,
  TreeStructure::internal_blanks_with_skipping,
  TreeStructure::unmerged_leaves_no_skipping,
  TreeStructure::unmerged_leaves_with_skipping,
};

std::array<TreeStructure, 11> treekem_test_tree_structures{
  // All cases except the big ones
  TreeStructure::full_tree_2,
  TreeStructure::full_tree_3,
  TreeStructure::full_tree_4,
  TreeStructure::full_tree_5,
  TreeStructure::full_tree_6,
  TreeStructure::full_tree_7,
  TreeStructure::full_tree_8,
  TreeStructure::internal_blanks_no_skipping,
  TreeStructure::internal_blanks_with_skipping,
  TreeStructure::unmerged_leaves_no_skipping,
  TreeStructure::unmerged_leaves_with_skipping,
};

struct TreeTestCase
{
  CipherSuite suite;
  PseudoRandom::Generator prg;

  bytes group_id;
  uint32_t leaf_counter = 0;
  uint32_t path_counter = 0;

  struct PrivateState
  {
    SignaturePrivateKey sig_priv;
    TreeKEMPrivateKey priv;
    std::vector<LeafIndex> senders;
  };

  std::map<LeafIndex, PrivateState> privs;
  TreeKEMPublicKey pub;

  TreeTestCase(CipherSuite suite_in, PseudoRandom::Generator prg_in)
    : suite(suite_in)
    , prg(std::move(prg_in))
    , group_id(prg.secret("group_id"))
    , pub(suite)
  {
    auto [where, enc_priv, sig_priv] = add_leaf();
    auto tree_priv = TreeKEMPrivateKey::solo(suite, where, enc_priv);
    auto priv_state = PrivateState{ sig_priv, tree_priv, { LeafIndex{ 0 } } };
    privs.insert_or_assign(where, priv_state);
  }

  std::tuple<LeafIndex, HPKEPrivateKey, SignaturePrivateKey> add_leaf()
  {
    leaf_counter += 1;
    auto ix = to_hex(tls::marshal(leaf_counter));
    auto enc_priv = prg.hpke_key("encryption_key" + ix);
    auto sig_priv = prg.signature_key("signature_key" + ix);
    auto identity = prg.secret("identity" + ix);

    auto credential = Credential::basic(identity);
    auto leaf_node = LeafNode{ suite,
                               enc_priv.public_key,
                               sig_priv.public_key,
                               credential,
                               Capabilities::create_default(),
                               Lifetime::create_default(),
                               {},
                               sig_priv };
    auto where = pub.add_leaf(leaf_node);
    pub.set_hash_all();
    return { where, enc_priv, sig_priv };
  }

  void commit(LeafIndex from,
              const std::vector<LeafIndex>& remove,
              bool add,
              std::optional<bytes> maybe_context)
  {
    // Remove members from the tree
    for (auto i : remove) {
      pub.blank_path(i);
      privs.erase(i);
    }
    pub.set_hash_all();

    auto joiner = std::vector<LeafIndex>{};
    auto maybe_enc_priv = std::optional<HPKEPrivateKey>{};
    auto maybe_sig_priv = std::optional<SignaturePrivateKey>{};
    if (add) {
      auto [where, enc_priv, sig_priv] = add_leaf();
      joiner.push_back(where);
      maybe_enc_priv = enc_priv;
      maybe_sig_priv = sig_priv;
    }

    auto path_secret = std::optional<bytes>{};
    if (maybe_context) {
      // Create an UpdatePath
      path_counter += 1;
      auto ix = to_hex(tls::marshal(path_counter));
      auto leaf_secret = prg.secret("leaf_secret" + ix);
      auto priv = privs.at(from);

      auto context = opt::get(maybe_context);
      auto pub_before = pub;
      auto sender_priv =
        pub.update(from, leaf_secret, group_id, priv.sig_priv, {});
      auto path = pub.encap(sender_priv, context, joiner);

      // Process the UpdatePath at all the members
      for (auto& pair : privs) {
        // XXX(RLB): It might seem like this could be done with a simple
        // destructuring assignment, either here or in the `for` clause above.
        // However, either of these options cause clang-tidy to segfault when
        // evaulating the "bugprone-unchecked-optional-access" lint.
        const auto& leaf = pair.first;
        auto& priv_state = pair.second;
        if (leaf == from) {
          priv_state =
            PrivateState{ priv_state.sig_priv, sender_priv, { from } };
          continue;
        }

        priv_state.priv.decap(from, pub_before, context, path, joiner);
        priv_state.senders.push_back(from);
      }

      // Look up the path secret for the joiner
      if (!joiner.empty()) {
        auto index = joiner.front();
        auto [overlap, shared_path_secret, ok] =
          sender_priv.shared_path_secret(index);
        silence_unused(overlap);
        silence_unused(ok);

        path_secret = shared_path_secret;
      }
    }

    // Add a private entry for the joiner if we added someone
    if (!joiner.empty()) {
      auto index = joiner.front();
      auto ancestor = index.ancestor(from);
      auto enc_priv = opt::get(maybe_enc_priv);
      auto sig_priv = opt::get(maybe_sig_priv);
      auto tree_priv =
        TreeKEMPrivateKey::joiner(pub, index, enc_priv, ancestor, path_secret);
      privs.insert_or_assign(index,
                             PrivateState{ sig_priv, tree_priv, { from } });
    }
  }

  static TreeTestCase full(CipherSuite suite,
                           const PseudoRandom::Generator& prg,
                           LeafCount leaves,
                           const std::string& label)
  {
    auto tc = TreeTestCase{ suite, prg.sub(label) };

    for (LeafIndex i{ 0 }; i.val < leaves.val - 1; i.val++) {
      tc.commit(
        i, {}, true, tc.prg.secret("context" + to_hex(tls::marshal(i))));
    }

    return tc;
  }

  static TreeTestCase with_structure(CipherSuite suite,
                                     const PseudoRandom::Generator& prg,
                                     TreeStructure tree_structure)
  {
    switch (tree_structure) {
      case TreeStructure::full_tree_2:
        return full(suite, prg, LeafCount{ 2 }, "full_tree_2");

      case TreeStructure::full_tree_3:
        return full(suite, prg, LeafCount{ 3 }, "full_tree_3");

      case TreeStructure::full_tree_4:
        return full(suite, prg, LeafCount{ 4 }, "full_tree_4");

      case TreeStructure::full_tree_5:
        return full(suite, prg, LeafCount{ 5 }, "full_tree_5");

      case TreeStructure::full_tree_6:
        return full(suite, prg, LeafCount{ 6 }, "full_tree_6");

      case TreeStructure::full_tree_7:
        return full(suite, prg, LeafCount{ 7 }, "full_tree_7");

      case TreeStructure::full_tree_8:
        return full(suite, prg, LeafCount{ 8 }, "full_tree_8");

      case TreeStructure::full_tree_32:
        return full(suite, prg, LeafCount{ 32 }, "full_tree_32");

      case TreeStructure::full_tree_33:
        return full(suite, prg, LeafCount{ 33 }, "full_tree_33");

      case TreeStructure::full_tree_34:
        return full(suite, prg, LeafCount{ 34 }, "full_tree_34");

      case TreeStructure::internal_blanks_no_skipping: {
        auto tc = TreeTestCase::full(
          suite, prg, LeafCount{ 8 }, "internal_blanks_no_skipping");
        auto context = tc.prg.secret("context");
        tc.commit(
          LeafIndex{ 0 }, { LeafIndex{ 2 }, LeafIndex{ 3 } }, true, context);
        return tc;
      }

      case TreeStructure::internal_blanks_with_skipping: {
        auto tc = TreeTestCase::full(
          suite, prg, LeafCount{ 8 }, "internal_blanks_with_skipping");
        auto context = tc.prg.secret("context");
        tc.commit(LeafIndex{ 0 },
                  { LeafIndex{ 1 }, LeafIndex{ 2 }, LeafIndex{ 3 } },
                  false,
                  context);
        return tc;
      }

      case TreeStructure::unmerged_leaves_no_skipping: {
        auto tc = TreeTestCase::full(
          suite, prg, LeafCount{ 7 }, "unmerged_leaves_no_skipping");
        auto context = tc.prg.secret("context");
        tc.commit(LeafIndex{ 0 }, {}, true, std::nullopt);
        return tc;
      }

      case TreeStructure::unmerged_leaves_with_skipping: {
        auto tc = TreeTestCase::full(
          suite, prg, LeafCount{ 1 }, "unmerged_leaves_with_skipping");

        // 0 adds 1..6
        tc.commit(LeafIndex{ 0 }, {}, true, std::nullopt);
        tc.commit(LeafIndex{ 0 }, {}, true, std::nullopt);
        tc.commit(LeafIndex{ 0 }, {}, true, std::nullopt);
        tc.commit(LeafIndex{ 0 }, {}, true, std::nullopt);
        tc.commit(LeafIndex{ 0 }, {}, true, std::nullopt);
        tc.commit(LeafIndex{ 0 }, {}, true, std::nullopt);

        // 0 reemoves 5
        tc.commit(LeafIndex{ 0 },
                  { LeafIndex{ 5 } },
                  false,
                  tc.prg.secret("context_remove5"));

        // 4 commits without any proupposals
        tc.commit(LeafIndex{ 4 }, {}, false, tc.prg.secret("context_update4"));

        // 0 adds a new member
        tc.commit(LeafIndex{ 0 }, {}, true, std::nullopt);

        return tc;
      }

      default:
        throw InvalidParameterError("Unsupported tree structure");
    }
  }
};

///
/// TreeHashTestVector
///
TreeHashTestVector::TreeHashTestVector(mlspp::CipherSuite suite,
                                       TreeStructure tree_structure)
  : PseudoRandom(suite, "tree-hashes")
  , cipher_suite(suite)
{
  auto tc = TreeTestCase::with_structure(suite, prg, tree_structure);
  tree = tc.pub;
  group_id = tc.group_id;

  auto width = NodeCount(tree.size);
  for (NodeIndex i{ 0 }; i < width; i.val++) {
    tree_hashes.push_back(tree.get_hash(i));
    resolutions.push_back(tree.resolve(i));
  }
}

std::optional<std::string>
TreeHashTestVector::verify()
{
  // Finish setting up the tree
  tree.suite = cipher_suite;
  tree.set_hash_all();

  // Verify that each leaf node is properly signed
  for (LeafIndex i{ 0 }; i < tree.size; i.val++) {
    auto maybe_leaf = tree.leaf_node(i);
    if (!maybe_leaf) {
      continue;
    }

    auto leaf = opt::get(maybe_leaf);
    auto leaf_valid = leaf.verify(cipher_suite, { { group_id, i } });
    VERIFY("leaf sig valid", leaf_valid);
  }

  // Verify the tree hashes
  auto width = NodeCount{ tree.size };
  for (NodeIndex i{ 0 }; i < width; i.val++) {
    VERIFY_EQUAL("tree hash", tree.get_hash(i), tree_hashes.at(i.val));
    VERIFY_EQUAL("resolution", tree.resolve(i), resolutions.at(i.val));
  }

  // Verify parent hashes
  VERIFY("parent hash valid", tree.parent_hash_valid());

  // Verify the resolutions
  for (NodeIndex i{ 0 }; i < width; i.val++) {
    VERIFY_EQUAL("resolution", tree.resolve(i), resolutions[i.val]);
  }

  return std::nullopt;
}

///
/// TreeOperationsTestVector
///

const std::vector<TreeOperationsTestVector::Scenario>
  TreeOperationsTestVector::all_scenarios{
    Scenario::add_right_edge,    Scenario::add_internal,    Scenario::update,
    Scenario::remove_right_edge, Scenario::remove_internal,
  };

TreeOperationsTestVector::TreeOperationsTestVector(
  mlspp::CipherSuite suite,
  Scenario scenario)
  : PseudoRandom(suite, "tree-operations")
  , cipher_suite(suite)
  , proposal_sender(0)
{
  auto init_priv = prg.hpke_key("init_key");
  auto enc_priv = prg.hpke_key("encryption_key");
  auto sig_priv = prg.signature_key("signature_key");
  auto identity = prg.secret("identity");
  auto credential = Credential::basic(identity);
  auto key_package = KeyPackage{
    suite,
    init_priv.public_key,
    { suite,
      enc_priv.public_key,
      sig_priv.public_key,
      credential,
      Capabilities::create_default(),
      Lifetime::create_default(),
      {},
      sig_priv },
    {},
    sig_priv,
  };

  switch (scenario) {
    case Scenario::add_right_edge: {
      auto tc = TreeTestCase::full(suite, prg, LeafCount{ 8 }, "tc");

      proposal = Proposal{ Add{ key_package } };

      tree_before = tc.pub;
      tree_hash_before = tree_before.root_hash();

      tree_after = tree_before;
      tree_after.add_leaf(key_package.leaf_node);
      break;
    }

    case Scenario::add_internal: {
      auto tc = TreeTestCase::full(suite, prg, LeafCount{ 8 }, "tc");

      proposal = Proposal{ Add{ key_package } };

      tree_before = tc.pub;
      tree_before.blank_path(LeafIndex{ 4 });
      tree_before.set_hash_all();
      tree_hash_before = tree_before.root_hash();

      tree_after = tree_before;
      tree_after.add_leaf(key_package.leaf_node);
      break;
    }

    case Scenario::update: {
      auto tc = TreeTestCase::full(suite, prg, LeafCount{ 8 }, "tc");

      proposal_sender = LeafIndex{ 3 };
      proposal = Proposal{ Update{ key_package.leaf_node } };

      tree_before = tc.pub;
      tree_hash_before = tree_before.root_hash();

      tree_after = tree_before;
      tree_after.update_leaf(proposal_sender, key_package.leaf_node);
      break;
    }

    case Scenario::remove_right_edge: {
      auto tc = TreeTestCase::full(suite, prg, LeafCount{ 9 }, "tc");

      auto removed = LeafIndex{ 8 };
      proposal = Proposal{ Remove{ removed } };

      tree_before = tc.pub;
      tree_hash_before = tree_before.root_hash();

      tree_after = tree_before;
      tree_after.blank_path(removed);
      tree_after.truncate();
      break;
    }

    case Scenario::remove_internal: {
      auto tc = TreeTestCase::full(suite, prg, LeafCount{ 8 }, "tc");

      auto removed = LeafIndex{ 4 };
      proposal = Proposal{ Remove{ removed } };

      tree_before = tc.pub;
      tree_hash_before = tree_before.root_hash();

      tree_after = tree_before;
      tree_after.blank_path(removed);
      tree_after.truncate();
      break;
    }
  }

  tree_after.set_hash_all();
  tree_hash_after = tree_after.root_hash();
}

std::optional<std::string>
TreeOperationsTestVector::verify()
{
  tree_before.suite = cipher_suite;
  tree_before.set_hash_all();

  auto tree = tree_before;
  VERIFY_EQUAL("tree hash before", tree.root_hash(), tree_hash_before);

  auto apply = overloaded{
    [&](const Add& add) { tree.add_leaf(add.key_package.leaf_node); },

    [&](const Update& update) {
      tree.update_leaf(proposal_sender, update.leaf_node);
    },

    [&](const Remove& remove) {
      tree.blank_path(remove.removed);
      tree.truncate();
    },

    [](const auto& /* other */) {
      throw InvalidParameterError("invalid proposal type");
    },
  };

  var::visit(apply, proposal.content);
  VERIFY_EQUAL("tree after", tree, tree_after);

  tree.set_hash_all();
  VERIFY_EQUAL("tree hash after", tree.root_hash(), tree_hash_after);

  return std::nullopt;
}

///
/// TreeKEMTestVector
///

TreeKEMTestVector::TreeKEMTestVector(mlspp::CipherSuite suite,
                                     TreeStructure tree_structure)
  : PseudoRandom(suite, "treekem")
  , cipher_suite(suite)
{
  auto tc = TreeTestCase::with_structure(cipher_suite, prg, tree_structure);

  group_id = tc.group_id;
  epoch = prg.uint64("epoch");
  confirmed_transcript_hash = prg.secret("confirmed_transcript_hash");

  ratchet_tree = tc.pub;

  // Serialize out the private states
  for (LeafIndex index{ 0 }; index < ratchet_tree.size; index.val++) {
    if (tc.privs.count(index) == 0) {
      continue;
    }

    auto priv_state = tc.privs.at(index);
    auto enc_priv = priv_state.priv.private_key_cache.at(NodeIndex(index));
    auto path_secrets = std::vector<PathSecret>{};
    for (const auto& [node, path_secret] : priv_state.priv.path_secrets) {
      if (node == NodeIndex(index)) {
        // No need to serialize a secret for the leaf node
        continue;
      }

      path_secrets.push_back(PathSecret{ node, path_secret });
    }

    leaves_private.push_back(LeafPrivateInfo{
      index,
      enc_priv,
      priv_state.sig_priv,
      path_secrets,
    });
  }

  // Create test update paths
  for (LeafIndex sender{ 0 }; sender < ratchet_tree.size; sender.val++) {
    if (!tc.pub.has_leaf(sender)) {
      continue;
    }

    auto leaf_secret = prg.secret("update_path" + to_hex(tls::marshal(sender)));
    const auto& sig_priv = tc.privs.at(sender).sig_priv;

    auto pub = tc.pub;
    auto new_sender_priv =
      pub.update(sender, leaf_secret, group_id, sig_priv, {});

    auto group_context = GroupContext{ cipher_suite,
                                       group_id,
                                       epoch,
                                       pub.root_hash(),
                                       confirmed_transcript_hash,
                                       {} };
    auto ctx = tls::marshal(group_context);

    auto path = pub.encap(new_sender_priv, ctx, {});

    auto path_secrets = std::vector<std::optional<bytes>>{};
    for (LeafIndex to{ 0 }; to < ratchet_tree.size; to.val++) {
      if (to == sender || !pub.has_leaf(to)) {
        path_secrets.emplace_back(std::nullopt);
        continue;
      }

      auto [overlap, path_secret, ok] = new_sender_priv.shared_path_secret(to);
      silence_unused(overlap);
      silence_unused(ok);

      path_secrets.emplace_back(path_secret);
    }

    update_paths.push_back(UpdatePathInfo{
      sender,
      path,
      path_secrets,
      new_sender_priv.update_secret,
      pub.root_hash(),
    });
  }
}

std::optional<std::string>
TreeKEMTestVector::verify()
{
  // Finish initializing the ratchet tree
  ratchet_tree.suite = cipher_suite;
  ratchet_tree.set_hash_all();

  // Validate public state
  VERIFY("parent hash valid", ratchet_tree.parent_hash_valid());

  for (LeafIndex i{ 0 }; i < ratchet_tree.size; i.val++) {
    auto maybe_leaf = ratchet_tree.leaf_node(i);
    if (!maybe_leaf) {
      continue;
    }

    auto leaf = opt::get(maybe_leaf);
    VERIFY("leaf sig", leaf.verify(cipher_suite, { { group_id, i } }));
  }

  // Import private keys
  std::map<LeafIndex, TreeKEMPrivateKey> tree_privs;
  std::map<LeafIndex, SignaturePrivateKey> sig_privs;
  for (const auto& info : leaves_private) {
    auto enc_priv = info.encryption_priv;
    auto sig_priv = info.signature_priv;
    enc_priv.set_public_key(cipher_suite);
    sig_priv.set_public_key(cipher_suite);

    auto priv = TreeKEMPrivateKey{};
    priv.suite = cipher_suite;
    priv.index = info.index;
    priv.private_key_cache.insert_or_assign(NodeIndex(info.index), enc_priv);

    for (const auto& entry : info.path_secrets) {
      priv.path_secrets.insert_or_assign(entry.node, entry.path_secret);
    }

    VERIFY("priv consistent", priv.consistent(ratchet_tree));

    tree_privs.insert_or_assign(info.index, priv);
    sig_privs.insert_or_assign(info.index, sig_priv);
  }

  for (const auto& info : update_paths) {
    // Test decap of the existing group secrets
    const auto& from = info.sender;
    const auto& path = info.update_path;
    VERIFY("path parent hash valid",
           ratchet_tree.parent_hash_valid(from, path));

    auto ratchet_tree_after = ratchet_tree;
    ratchet_tree_after.merge(from, path);
    ratchet_tree_after.set_hash_all();
    VERIFY_EQUAL(
      "tree hash after", ratchet_tree_after.root_hash(), info.tree_hash_after);

    auto group_context = GroupContext{ cipher_suite,
                                       group_id,
                                       epoch,
                                       ratchet_tree_after.root_hash(),
                                       confirmed_transcript_hash,
                                       {} };
    auto ctx = tls::marshal(group_context);

    for (LeafIndex to{ 0 }; to < ratchet_tree_after.size; to.val++) {
      if (to == from || !ratchet_tree_after.has_leaf(to)) {
        continue;
      }

      auto priv = tree_privs.at(to);
      priv.decap(from, ratchet_tree_after, ctx, path, {});
      VERIFY_EQUAL("commit secret", priv.update_secret, info.commit_secret);

      auto [overlap, path_secret, ok] = priv.shared_path_secret(from);
      silence_unused(overlap);
      silence_unused(ok);
      VERIFY_EQUAL("path secret", path_secret, info.path_secrets[to.val]);
    }

    // Test encap/decap
    auto ratchet_tree_encap = ratchet_tree;
    auto leaf_secret = random_bytes(cipher_suite.secret_size());
    const auto& sig_priv = sig_privs.at(from);
    auto new_sender_priv =
      ratchet_tree_encap.update(from, leaf_secret, group_id, sig_priv, {});
    auto new_path = ratchet_tree_encap.encap(new_sender_priv, ctx, {});
    VERIFY("new path parent hash valid",
           ratchet_tree.parent_hash_valid(from, path));

    for (LeafIndex to{ 0 }; to < ratchet_tree_encap.size; to.val++) {
      if (to == from || !ratchet_tree_encap.has_leaf(to)) {
        continue;
      }

      auto priv = tree_privs.at(to);
      priv.decap(from, ratchet_tree_encap, ctx, new_path, {});
      VERIFY_EQUAL(
        "commit secret", priv.update_secret, new_sender_priv.update_secret);
    }
  }

  return std::nullopt;
}

///
/// MessagesTestVector
///

MessagesTestVector::MessagesTestVector()
  : PseudoRandom(CipherSuite::ID::X25519_AES128GCM_SHA256_Ed25519, "messages")
{
  auto suite = CipherSuite{ CipherSuite::ID::X25519_AES128GCM_SHA256_Ed25519 };
  auto epoch = epoch_t(prg.uint64("epoch"));
  auto index = LeafIndex{ prg.uint32("index") };
  auto user_id = prg.secret("user_id");
  auto group_id = prg.secret("group_id");
  // auto opaque = bytes(32, 0xD3);
  // auto mac = bytes(32, 0xD5);

  auto app_id_ext = ApplicationIDExtension{ prg.secret("app_id") };
  auto ext_list = ExtensionList{};
  ext_list.add(app_id_ext);

  auto group_context = GroupContext{ suite,
                                     group_id,
                                     epoch,
                                     prg.secret("tree_hash"),
                                     prg.secret("confirmed_trasncript_hash"),
                                     ext_list };

  auto version = ProtocolVersion::mls10;
  auto hpke_priv = prg.hpke_key("hpke_priv");
  auto hpke_priv_2 = prg.hpke_key("hpke_priv_2");
  auto hpke_pub = hpke_priv.public_key;
  auto hpke_pub_2 = hpke_priv_2.public_key;
  auto hpke_ct =
    HPKECiphertext{ prg.secret("kem_output"), prg.secret("ciphertext") };
  auto sig_priv = prg.signature_key("signature_priv");
  auto sig_priv_2 = prg.signature_key("signature_priv_2");
  auto sig_pub = sig_priv.public_key;
  auto sig_pub_2 = sig_priv_2.public_key;

  // KeyPackage and extensions
  auto cred = Credential::basic(user_id);
  auto leaf_node = LeafNode{ suite,
                             hpke_pub,
                             sig_pub,
                             cred,
                             Capabilities::create_default(),
                             Lifetime::create_default(),
                             ext_list,
                             sig_priv };
  auto leaf_node_2 = LeafNode{ suite,
                               hpke_pub_2,
                               sig_pub_2,
                               cred,
                               Capabilities::create_default(),
                               Lifetime::create_default(),
                               ext_list,
                               sig_priv_2 };
  auto key_package_obj = KeyPackage{ suite, hpke_pub, leaf_node, {}, sig_priv };

  auto leaf_node_update =
    leaf_node.for_update(suite, group_id, index, hpke_pub, {}, sig_priv);
  auto leaf_node_commit = leaf_node.for_commit(
    suite, group_id, index, hpke_pub, prg.secret("parent_hash"), {}, sig_priv);

  auto sender = Sender{ MemberSender{ index } };

  auto tree = TreeKEMPublicKey{ suite };
  tree.add_leaf(leaf_node);
  tree.add_leaf(leaf_node_2);
  auto ratchet_tree_obj = RatchetTreeExtension{ tree };

  // Welcome and its substituents
  auto group_info_obj =
    GroupInfo{ group_context, ext_list, prg.secret("confirmation_tag") };
  auto joiner_secret = prg.secret("joiner_secret");
  auto path_secret = prg.secret("path_secret");
  auto psk_id = ExternalPSK{ prg.secret("psk_id") };
  auto psk_nonce = prg.secret("psk_nonce");
  auto group_secrets_obj = GroupSecrets{ joiner_secret,
                                         { { path_secret } },
                                         PreSharedKeys{ {
                                           { psk_id, psk_nonce },
                                         } } };
  auto welcome_obj = Welcome{ suite, joiner_secret, {}, group_info_obj };
  welcome_obj.encrypt(key_package_obj, path_secret);

  // Proposals
  auto add = Add{ key_package_obj };
  auto update = Update{ leaf_node_update };
  auto remove = Remove{ index };
  auto pre_shared_key = PreSharedKey{ psk_id, psk_nonce };
  auto reinit = ReInit{ group_id, version, suite, {} };
  auto external_init = ExternalInit{ prg.secret("external_init") };

  // Commit
  auto proposal_ref = ProposalRef{ 32, 0xa0 };

  auto commit_obj = Commit{ {
                              { proposal_ref },
                              { Proposal{ add } },
                            },
                            UpdatePath{
                              leaf_node_commit,
                              {
                                { hpke_pub, { hpke_ct, hpke_ct } },
                                { hpke_pub, { hpke_ct, hpke_ct, hpke_ct } },
                              },
                            } };

  // AuthenticatedContent with Application / Proposal / Commit

  // PublicMessage
  auto membership_key = prg.secret("membership_key");

  auto content_auth_proposal = AuthenticatedContent::sign(
    WireFormat::mls_public_message,
    { group_id, epoch, sender, {}, Proposal{ remove } },
    suite,
    sig_priv,
    group_context);
  auto public_message_proposal_obj = PublicMessage::protect(
    content_auth_proposal, suite, membership_key, group_context);

  auto content_auth_commit =
    AuthenticatedContent::sign(WireFormat::mls_public_message,
                               { group_id, epoch, sender, {}, commit_obj },
                               suite,
                               sig_priv,
                               group_context);
  content_auth_commit.set_confirmation_tag(prg.secret("confirmation_tag"));
  auto public_message_commit_obj = PublicMessage::protect(
    content_auth_commit, suite, membership_key, group_context);

  // PrivateMessage
  auto content_auth_application_obj = AuthenticatedContent::sign(
    WireFormat::mls_private_message,
    { group_id, epoch, sender, {}, ApplicationData{} },
    suite,
    sig_priv,
    group_context);

  auto keys = GroupKeySource(
    suite, LeafCount{ index.val + 1 }, prg.secret("encryption_secret"));
  auto private_message_obj =
    PrivateMessage::protect(content_auth_application_obj,
                            suite,
                            keys,
                            prg.secret("sender_data_secret"),
                            10);

  // Serialize out all the objects
  mls_welcome = tls::marshal(MLSMessage{ welcome_obj });
  mls_group_info = tls::marshal(MLSMessage{ group_info_obj });
  mls_key_package = tls::marshal(MLSMessage{ key_package_obj });

  ratchet_tree = tls::marshal(ratchet_tree_obj);
  group_secrets = tls::marshal(group_secrets_obj);

  add_proposal = tls::marshal(add);
  update_proposal = tls::marshal(update);
  remove_proposal = tls::marshal(remove);
  pre_shared_key_proposal = tls::marshal(pre_shared_key);
  re_init_proposal = tls::marshal(reinit);
  external_init_proposal = tls::marshal(external_init);

  commit = tls::marshal(commit_obj);

  public_message_proposal =
    tls::marshal(MLSMessage{ public_message_proposal_obj });
  public_message_commit = tls::marshal(MLSMessage{ public_message_commit_obj });
  private_message = tls::marshal(MLSMessage{ private_message_obj });
}

std::optional<std::string>
MessagesTestVector::verify() const
{
  // TODO(RLB) Verify signatures
  // TODO(RLB) Verify content types in PublicMessage objects
  auto require_format = [](WireFormat format) {
    return
      [format](const MLSMessage& msg) { return msg.wire_format() == format; };
  };

  VERIFY_TLS_RTT_VAL("Welcome",
                     MLSMessage,
                     mls_welcome,
                     require_format(WireFormat::mls_welcome));
  VERIFY_TLS_RTT_VAL("GroupInfo",
                     MLSMessage,
                     mls_group_info,
                     require_format(WireFormat::mls_group_info));
  VERIFY_TLS_RTT_VAL("KeyPackage",
                     MLSMessage,
                     mls_key_package,
                     require_format(WireFormat::mls_key_package));

  VERIFY_TLS_RTT("RatchetTree", RatchetTreeExtension, ratchet_tree);
  VERIFY_TLS_RTT("GroupSecrets", GroupSecrets, group_secrets);

  VERIFY_TLS_RTT("Add", Add, add_proposal);
  VERIFY_TLS_RTT("Update", Update, update_proposal);
  VERIFY_TLS_RTT("Remove", Remove, remove_proposal);
  VERIFY_TLS_RTT("PreSharedKey", PreSharedKey, pre_shared_key_proposal);
  VERIFY_TLS_RTT("ReInit", ReInit, re_init_proposal);
  VERIFY_TLS_RTT("ExternalInit", ExternalInit, external_init_proposal);

  VERIFY_TLS_RTT("Commit", Commit, commit);

  VERIFY_TLS_RTT_VAL("Public(Proposal)",
                     MLSMessage,
                     public_message_proposal,
                     require_format(WireFormat::mls_public_message));
  VERIFY_TLS_RTT_VAL("Public(Commit)",
                     MLSMessage,
                     public_message_commit,
                     require_format(WireFormat::mls_public_message));
  VERIFY_TLS_RTT_VAL("PrivateMessage",
                     MLSMessage,
                     private_message,
                     require_format(WireFormat::mls_private_message));

  return std::nullopt;
}

std::optional<std::string>
PassiveClientTestVector::verify()
{
  // Import everything
  signature_priv.set_public_key(cipher_suite);
  encryption_priv.set_public_key(cipher_suite);
  init_priv.set_public_key(cipher_suite);

  const auto& key_package_raw = var::get<KeyPackage>(key_package.message);
  const auto& welcome_raw = var::get<Welcome>(welcome.message);

  auto ext_psks = std::map<bytes, bytes>{};
  for (const auto& [id, psk] : external_psks) {
    ext_psks.insert_or_assign(id, psk);
  }

  // Join the group and follow along
  auto state = State(init_priv,
                     encryption_priv,
                     signature_priv,
                     key_package_raw,
                     welcome_raw,
                     ratchet_tree,
                     ext_psks);
  VERIFY_EQUAL(
    "initial epoch", state.epoch_authenticator(), initial_epoch_authenticator);

  for (const auto& tve : epochs) {
    for (const auto& proposal : tve.proposals) {
      state.handle(proposal);
    }

    state = opt::get(state.handle(tve.commit));
    VERIFY_EQUAL(
      "epoch auth", state.epoch_authenticator(), tve.epoch_authenticator)
  }

  return std::nullopt;
}

} // namespace mls_vectors
