#include <mls/messages.h>
#include <mls/session.h>

#include <deque>

namespace mlspp {

///
/// Inner struct declarations for PendingJoin and Session
///

struct PendingJoin::Inner
{
  const CipherSuite suite;
  const HPKEPrivateKey init_priv;
  const HPKEPrivateKey leaf_priv;
  const SignaturePrivateKey sig_priv;
  const KeyPackage key_package;

  Inner(CipherSuite suite_in,
        SignaturePrivateKey sig_priv_in,
        Credential cred_in);

  static PendingJoin create(CipherSuite suite,
                            SignaturePrivateKey sig_priv,
                            Credential cred);
};

struct Session::Inner
{
  std::deque<State> history;
  std::map<bytes, State> outbound_cache;
  bool encrypt_handshake{ false };

  explicit Inner(State state);

  static Session begin(CipherSuite suite,
                       const bytes& group_id,
                       const HPKEPrivateKey& leaf_priv,
                       const SignaturePrivateKey& sig_priv,
                       const LeafNode& leaf_node);
  static Session join(const HPKEPrivateKey& init_priv,
                      const HPKEPrivateKey& leaf_priv,
                      const SignaturePrivateKey& sig_priv,
                      const KeyPackage& key_package,
                      const bytes& welcome_data);

  bytes fresh_secret() const;
  MLSMessage import_handshake(const bytes& encoded) const;
  State& for_epoch(epoch_t epoch);
};

///
/// Client
///

Client::Client(CipherSuite suite_in,
               SignaturePrivateKey sig_priv_in,
               Credential cred_in)
  : suite(suite_in)
  , sig_priv(std::move(sig_priv_in))
  , cred(std::move(cred_in))
{
}

Session
Client::begin_session(const bytes& group_id) const
{
  auto leaf_priv = HPKEPrivateKey::generate(suite);
  auto leaf_node = LeafNode(suite,
                            leaf_priv.public_key,
                            sig_priv.public_key,
                            cred,
                            Capabilities::create_default(),
                            Lifetime::create_default(),
                            {},
                            sig_priv);

  return Session::Inner::begin(suite, group_id, leaf_priv, sig_priv, leaf_node);
}

PendingJoin
Client::start_join() const
{
  return PendingJoin::Inner::create(suite, sig_priv, cred);
}

///
/// PendingJoin
///

PendingJoin::Inner::Inner(CipherSuite suite_in,
                          SignaturePrivateKey sig_priv_in,
                          Credential cred_in)
  : suite(suite_in)
  , init_priv(HPKEPrivateKey::generate(suite))
  , leaf_priv(HPKEPrivateKey::generate(suite))
  , sig_priv(std::move(sig_priv_in))
  , key_package(suite,
                init_priv.public_key,
                LeafNode(suite,
                         leaf_priv.public_key,
                         sig_priv.public_key,
                         std::move(cred_in),
                         Capabilities::create_default(),
                         Lifetime::create_default(),
                         {},
                         sig_priv),
                {},
                sig_priv)
{
}

PendingJoin
PendingJoin::Inner::create(CipherSuite suite,
                           SignaturePrivateKey sig_priv,
                           Credential cred)
{
  auto inner =
    std::make_unique<Inner>(suite, std::move(sig_priv), std::move(cred));
  return { inner.release() };
}

PendingJoin::PendingJoin(PendingJoin&& other) noexcept = default;

PendingJoin&
PendingJoin::operator=(PendingJoin&& other) noexcept = default;

PendingJoin::~PendingJoin() = default;

PendingJoin::PendingJoin(Inner* inner_in)
  : inner(inner_in)
{
}

bytes
PendingJoin::key_package() const
{
  return tls::marshal(inner->key_package);
}

Session
PendingJoin::complete(const bytes& welcome) const
{
  return Session::Inner::join(inner->init_priv,
                              inner->leaf_priv,
                              inner->sig_priv,
                              inner->key_package,
                              welcome);
}

///
/// Session
///

Session::Inner::Inner(State state)
  : history{ std::move(state) }
  , encrypt_handshake(true)
{
}

Session
Session::Inner::begin(CipherSuite suite,
                      const bytes& group_id,
                      const HPKEPrivateKey& leaf_priv,
                      const SignaturePrivateKey& sig_priv,
                      const LeafNode& leaf_node)
{
  auto state = State(group_id, suite, leaf_priv, sig_priv, leaf_node, {});
  auto inner = std::make_unique<Inner>(state);
  return { inner.release() };
}

Session
Session::Inner::join(const HPKEPrivateKey& init_priv,
                     const HPKEPrivateKey& leaf_priv,
                     const SignaturePrivateKey& sig_priv,
                     const KeyPackage& key_package,
                     const bytes& welcome_data)
{
  auto welcome = tls::get<Welcome>(welcome_data);

  auto state = State(
    init_priv, leaf_priv, sig_priv, key_package, welcome, std::nullopt, {});
  auto inner = std::make_unique<Inner>(state);
  return { inner.release() };
}

bytes
Session::Inner::fresh_secret() const
{
  const auto suite = history.front().cipher_suite();
  return random_bytes(suite.secret_size());
}

MLSMessage
Session::Inner::import_handshake(const bytes& encoded) const
{
  auto msg = tls::get<MLSMessage>(encoded);

  switch (msg.wire_format()) {
    case WireFormat::mls_public_message:
      if (encrypt_handshake) {
        throw ProtocolError("Handshake not encrypted as required");
      }

      return msg;

    case WireFormat::mls_private_message: {
      if (!encrypt_handshake) {
        throw ProtocolError("Unexpected handshake encryption");
      }

      return msg;
    }

    default:
      throw InvalidParameterError("Illegal wire format");
  }
}

State&
Session::Inner::for_epoch(epoch_t epoch)
{
  for (auto& state : history) {
    if (state.epoch() == epoch) {
      return state;
    }
  }

  throw MissingStateError("No state for epoch");
}

Session::Session(Session&& other) noexcept = default;

Session&
Session::operator=(Session&& other) noexcept = default;

Session::~Session() = default;

Session::Session(Inner* inner_in)
  : inner(inner_in)
{
}

void
Session::encrypt_handshake(bool enabled)
{
  inner->encrypt_handshake = enabled;
}

bytes
Session::add(const bytes& key_package_data)
{
  auto key_package = tls::get<KeyPackage>(key_package_data);
  auto proposal = inner->history.front().add(
    key_package, { inner->encrypt_handshake, {}, 0 });
  return tls::marshal(proposal);
}

bytes
Session::update()
{
  auto leaf_secret = inner->fresh_secret();

  auto leaf_priv = HPKEPrivateKey::generate(cipher_suite());
  auto proposal = inner->history.front().update(
    std::move(leaf_priv), {}, { inner->encrypt_handshake, {}, 0 });
  return tls::marshal(proposal);
}

bytes
Session::remove(uint32_t index)
{
  auto proposal = inner->history.front().remove(
    RosterIndex{ index }, { inner->encrypt_handshake, {}, 0 });
  return tls::marshal(proposal);
}

std::tuple<bytes, bytes>
Session::commit(const bytes& proposal)
{
  return commit(std::vector<bytes>{ proposal });
}

std::tuple<bytes, bytes>
Session::commit(const std::vector<bytes>& proposals)
{
  auto provisional_state = inner->history.front();
  for (const auto& proposal_data : proposals) {
    auto msg = inner->import_handshake(proposal_data);
    auto maybe_state = provisional_state.handle(msg);
    if (maybe_state) {
      throw InvalidParameterError("Invalid proposal; actually a commit");
    }
  }

  inner->history.front() = std::move(provisional_state);
  return commit();
}

std::tuple<bytes, bytes>
Session::commit()
{
  auto commit_secret = inner->fresh_secret();
  auto encrypt = inner->encrypt_handshake;
  auto [commit, welcome, new_state] = inner->history.front().commit(
    commit_secret, CommitOpts{ {}, true, encrypt, {} }, { encrypt, {}, 0 });

  auto commit_msg = tls::marshal(commit);
  auto welcome_msg = tls::marshal(welcome);

  inner->outbound_cache.insert({ commit_msg, new_state });
  return std::make_tuple(welcome_msg, commit_msg);
}

bool
Session::handle(const bytes& handshake_data)
{
  auto msg = inner->import_handshake(handshake_data);

  auto maybe_cached_state = std::optional<State>{};
  auto node = inner->outbound_cache.extract(handshake_data);
  if (!node.empty()) {
    maybe_cached_state = node.mapped();
  }

  auto maybe_next_state =
    inner->history.front().handle(msg, maybe_cached_state);
  if (!maybe_next_state) {
    return false;
  }

  inner->history.emplace_front(opt::get(maybe_next_state));
  return true;
}

epoch_t
Session::epoch() const
{
  return inner->history.front().epoch();
}

LeafIndex
Session::index() const
{
  return inner->history.front().index();
}

CipherSuite
Session::cipher_suite() const
{
  return inner->history.front().cipher_suite();
}

const ExtensionList&
Session::extensions() const
{
  return inner->history.front().extensions();
}

const TreeKEMPublicKey&
Session::tree() const
{
  return inner->history.front().tree();
}

bytes
Session::do_export(const std::string& label,
                   const bytes& context,
                   size_t size) const
{
  return inner->history.front().do_export(label, context, size);
}

GroupInfo
Session::group_info() const
{
  return inner->history.front().group_info(true);
}

std::vector<LeafNode>
Session::roster() const
{
  return inner->history.front().roster();
}

bytes
Session::epoch_authenticator() const
{
  return inner->history.front().epoch_authenticator();
}

bytes
Session::protect(const bytes& plaintext)
{
  auto msg = inner->history.front().protect({}, plaintext, 0);
  return tls::marshal(msg);
}

// TODO(rlb@ipv.sx): It would be good to expose identity information
// here, since ciphertexts are authenticated per sender.  Who sent
// this ciphertext?
bytes
Session::unprotect(const bytes& ciphertext)
{
  auto ciphertext_obj = tls::get<MLSMessage>(ciphertext);
  auto& state = inner->for_epoch(ciphertext_obj.epoch());
  auto [aad, pt] = state.unprotect(ciphertext_obj);
  silence_unused(aad);
  return pt;
}

bool
operator==(const Session& lhs, const Session& rhs)
{
  if (lhs.inner->encrypt_handshake != rhs.inner->encrypt_handshake) {
    return false;
  }

  auto size = std::min(lhs.inner->history.size(), rhs.inner->history.size());
  for (size_t i = 0; i < size; i += 1) {
    if (lhs.inner->history.at(i) != rhs.inner->history.at(i)) {
      return false;
    }
  }

  return true;
}

bool
operator!=(const Session& lhs, const Session& rhs)
{
  return !(lhs == rhs);
}

} // namespace mlspp
