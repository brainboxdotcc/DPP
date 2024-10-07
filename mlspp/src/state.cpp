#include <mls/state.h>
#include <set>

namespace mlspp {

///
/// Constructors
///

State::State(bytes group_id,
             CipherSuite suite,
             HPKEPrivateKey enc_priv,
             SignaturePrivateKey sig_priv,
             const LeafNode& leaf_node,
             ExtensionList extensions)
  : _suite(suite)
  , _group_id(std::move(group_id))
  , _epoch(0)
  , _tree(suite)
  , _transcript_hash(suite)
  , _extensions(std::move(extensions))
  , _index(0)
  , _identity_priv(std::move(sig_priv))
{
  // Verify that the client supports the proposed group extensions
  if (!leaf_node.verify_extension_support(_extensions)) {
    throw InvalidParameterError("Client doesn't support required extensions");
  }

  _index = _tree.add_leaf(leaf_node);
  _tree.set_hash_all();
  _tree_priv = TreeKEMPrivateKey::solo(suite, _index, std::move(enc_priv));
  if (!_tree_priv.consistent(_tree)) {
    throw InvalidParameterError("LeafNode inconsistent with private key");
  }

  // XXX(RLB): Convert KeyScheduleEpoch to take GroupContext?
  auto ctx = tls::marshal(group_context());
  _key_schedule =
    KeyScheduleEpoch(_suite, random_bytes(_suite.secret_size()), ctx);
  _keys = _key_schedule.encryption_keys(_tree.size);

  // Update the interim transcript hash with a virtual confirmation tag
  _transcript_hash.update_interim(
    _key_schedule.confirmation_tag(_transcript_hash.confirmed));
}

TreeKEMPublicKey
State::import_tree(const bytes& tree_hash,
                   const std::optional<TreeKEMPublicKey>& external,
                   const ExtensionList& extensions)
{
  auto tree = TreeKEMPublicKey(_suite);
  auto maybe_tree_extn = extensions.find<RatchetTreeExtension>();
  if (external) {
    tree = opt::get(external);
  } else if (maybe_tree_extn) {
    tree = opt::get(maybe_tree_extn).tree;
  } else {
    throw InvalidParameterError("No tree available");
  }

  tree.suite = _suite;

  tree.set_hash_all();
  if (tree.root_hash() != tree_hash) {
    throw InvalidParameterError("Tree does not match GroupInfo");
  }

  return tree;
}

bool
State::validate_tree() const
{
  // The functionality here is somewhat duplicative of State::valid(const
  // LeafNode&).  Simply calling that method, however, would result in this
  // method having quadratic scaling, since each call to valid() does a linear
  // scan through the tree to check uniqueness of keys and compatibility of
  // credential support.

  // Validate that the tree is parent-hash valid
  if (!_tree.parent_hash_valid()) {
    return false;
  }

  // Validate the signatures on all leaves
  const auto signature_valid =
    _tree.all_leaves([&](auto i, const auto& leaf_node) {
      auto binding = std::optional<LeafNode::MemberBinding>{};
      switch (leaf_node.source()) {
        case LeafNodeSource::commit:
        case LeafNodeSource::update:
          binding = LeafNode::MemberBinding{ _group_id, i };
          break;

        default:
          // Nothing to do
          break;
      }

      return leaf_node.verify(_suite, binding);
    });
  if (!signature_valid) {
    return false;
  }

  // Collect cross-tree properties
  auto n_leaves = size_t(0);
  auto encryption_keys = std::set<bytes>{};
  auto signature_keys = std::set<bytes>{};
  auto credential_types = std::set<CredentialType>{};
  _tree.all_leaves([&](auto /* i */, const auto& leaf_node) {
    n_leaves += 1;
    encryption_keys.insert(leaf_node.encryption_key.data);
    signature_keys.insert(leaf_node.signature_key.data);
    credential_types.insert(leaf_node.credential.type());
    return true;
  });

  // Verify uniqueness of keys
  if (encryption_keys.size() != n_leaves) {
    return false;
  }

  if (signature_keys.size() != n_leaves) {
    return false;
  }

  // Verify that each leaf indicates support for all required parameters
  return _tree.all_leaves([&](auto /* i */, const auto& leaf_node) {
    const auto supports_group_extensions =
      leaf_node.verify_extension_support(_extensions);
    const auto supports_own_extensions =
      leaf_node.verify_extension_support(leaf_node.extensions);
    const auto supports_group_credentials =
      leaf_node.capabilities.credentials_supported(credential_types);
    return supports_group_extensions && supports_own_extensions &&
           supports_group_credentials;
  });
}

State::State(SignaturePrivateKey sig_priv,
             const GroupInfo& group_info,
             const std::optional<TreeKEMPublicKey>& tree)
  : _suite(group_info.group_context.cipher_suite)
  , _group_id(group_info.group_context.group_id)
  , _epoch(group_info.group_context.epoch)
  , _tree(import_tree(group_info.group_context.tree_hash,
                      tree,
                      group_info.extensions))
  , _transcript_hash(_suite,
                     group_info.group_context.confirmed_transcript_hash,
                     group_info.confirmation_tag)
  , _extensions(group_info.group_context.extensions)
  , _key_schedule(_suite)
  , _index(0)
  , _identity_priv(std::move(sig_priv))
{
  if (!validate_tree()) {
    throw InvalidParameterError("Invalid tree");
  }

  // The following are not set:
  //    _index
  //    _tree_priv
  //
  // This ctor should only be used within external_commit, in which case these
  // fields are populated by the subsequent commit()
}

// Initialize a group from a Welcome
State::State(const HPKEPrivateKey& init_priv,
             HPKEPrivateKey leaf_priv,
             SignaturePrivateKey sig_priv,
             const KeyPackage& key_package,
             const Welcome& welcome,
             const std::optional<TreeKEMPublicKey>& tree,
             std::map<bytes, bytes> external_psks)
  : State(init_priv,
          std::move(leaf_priv),
          std::move(sig_priv),
          key_package,
          welcome,
          tree,
          std::move(external_psks),
          {})
{
}

State::State(const HPKEPrivateKey& init_priv,
             HPKEPrivateKey leaf_priv,
             SignaturePrivateKey sig_priv,
             const KeyPackage& key_package,
             const Welcome& welcome,
             const std::optional<TreeKEMPublicKey>& tree,
             std::map<bytes, bytes> external_psks,
             std::map<EpochRef, bytes> resumption_psks)
  : _suite(welcome.cipher_suite)
  , _epoch(0)
  , _tree(welcome.cipher_suite)
  , _transcript_hash(welcome.cipher_suite)
  , _identity_priv(std::move(sig_priv))
  , _external_psks(std::move(external_psks))
  , _resumption_psks(std::move(resumption_psks))
{
  auto maybe_kpi = welcome.find(key_package);
  if (!maybe_kpi) {
    throw InvalidParameterError("Welcome not intended for key package");
  }
  auto kpi = opt::get(maybe_kpi);

  if (key_package.cipher_suite != welcome.cipher_suite) {
    throw InvalidParameterError("Ciphersuite mismatch");
  }

  // Decrypt the GroupSecrets and look up required PSKs
  auto secrets = welcome.decrypt_secrets(kpi, init_priv);
  auto psks = resolve(secrets.psks.psks);

  // Decrypt the GroupInfo
  auto group_info = welcome.decrypt(secrets.joiner_secret, psks);
  if (group_info.group_context.cipher_suite != _suite) {
    throw InvalidParameterError("GroupInfo and Welcome ciphersuites disagree");
  }

  // Import the tree from the argument or from the extension
  _tree = import_tree(
    group_info.group_context.tree_hash, tree, group_info.extensions);

  // Verify the signature on the GroupInfo
  if (!group_info.verify(_tree)) {
    throw InvalidParameterError("Invalid GroupInfo");
  }

  // Ingest the GroupSecrets and GroupInfo
  _epoch = group_info.group_context.epoch;
  _group_id = group_info.group_context.group_id;

  _transcript_hash.confirmed =
    group_info.group_context.confirmed_transcript_hash;
  _transcript_hash.update_interim(group_info.confirmation_tag);

  _extensions = group_info.group_context.extensions;

  // Validate that the tree is in fact consistent with the group's parameters
  if (!validate_tree()) {
    throw InvalidParameterError("Invalid tree");
  }

  // Construct TreeKEM private key from parts provided
  auto maybe_index = _tree.find(key_package.leaf_node);
  if (!maybe_index) {
    throw InvalidParameterError("New joiner not in tree");
  }

  _index = opt::get(maybe_index);

  auto ancestor = _index.ancestor(group_info.signer);
  auto path_secret = std::optional<bytes>{};
  if (secrets.path_secret) {
    path_secret = opt::get(secrets.path_secret).secret;
  }

  _tree_priv = TreeKEMPrivateKey::joiner(
    _tree, _index, std::move(leaf_priv), ancestor, path_secret);

  // Ratchet forward into the current epoch
  auto group_ctx = tls::marshal(group_context());
  _key_schedule =
    KeyScheduleEpoch::joiner(_suite, secrets.joiner_secret, psks, group_ctx);
  _keys = _key_schedule.encryption_keys(_tree.size);

  // Verify the confirmation
  const auto confirmation_tag =
    _key_schedule.confirmation_tag(_transcript_hash.confirmed);
  if (confirmation_tag != group_info.confirmation_tag) {
    throw ProtocolError("Confirmation failed to verify");
  }
}

std::tuple<MLSMessage, State>
State::external_join(const bytes& leaf_secret,
                     SignaturePrivateKey sig_priv,
                     const KeyPackage& key_package,
                     const GroupInfo& group_info,
                     const std::optional<TreeKEMPublicKey>& tree,
                     const MessageOpts& msg_opts,
                     std::optional<LeafIndex> remove_prior,
                     const std::map<bytes, bytes>& psks)
{
  // Create a preliminary state
  auto initial_state = State(std::move(sig_priv), group_info, tree);

  // Look up the external public key for the group
  const auto maybe_external_pub =
    group_info.extensions.find<ExternalPubExtension>();
  if (!maybe_external_pub) {
    throw InvalidParameterError("No external pub in GroupInfo");
  }

  const auto& external_pub = opt::get(maybe_external_pub).external_pub;

  // Insert an ExternalInit proposal
  auto opts = CommitOpts{};
  auto [enc, force_init_secret] =
    KeyScheduleEpoch::external_init(key_package.cipher_suite, external_pub);
  auto ext_init = Proposal{ ExternalInit{ enc } };
  opts.extra_proposals.push_back(ext_init);

  // Evict a prior appearance if required
  if (remove_prior) {
    auto remove = initial_state.remove_proposal(opt::get(remove_prior));
    opts.extra_proposals.push_back(remove);
  }

  // Inject PSKs
  for (const auto& [id, secret] : psks) {
    initial_state.add_external_psk(id, secret);
    auto psk = initial_state.pre_shared_key_proposal(id);
    opts.extra_proposals.push_back(psk);
  }

  // Use the preliminary state to create a commit and advance to a real state
  auto params = ExternalCommitParams{ key_package, force_init_secret };
  auto [commit_msg, welcome, state] =
    initial_state.commit(leaf_secret, opts, msg_opts, params);
  silence_unused(welcome);
  return { commit_msg, state };
}

MLSMessage
State::new_member_add(const bytes& group_id,
                      epoch_t epoch,
                      const KeyPackage& new_member,
                      const SignaturePrivateKey& sig_priv)
{
  const auto suite = new_member.cipher_suite;
  auto proposal = Proposal{ Add{ new_member } };
  auto content = GroupContent{ group_id,
                               epoch,
                               { NewMemberProposalSender{} },
                               { /* no authenticated data */ },
                               { std::move(proposal) } };
  auto content_auth = AuthenticatedContent::sign(
    WireFormat::mls_public_message, std::move(content), suite, sig_priv, {});

  return PublicMessage::protect(std::move(content_auth), suite, {}, {});
}

///
/// Proposal and commit factories
///
template<typename Inner>
MLSMessage
State::protect_full(Inner&& inner_content, const MessageOpts& msg_opts)
{
  auto content_auth = sign({ MemberSender{ _index } },
                           std::forward<Inner>(inner_content),
                           msg_opts.authenticated_data,
                           msg_opts.encrypt);
  return protect(std::move(content_auth), msg_opts.padding_size);
}

template<typename Inner>
AuthenticatedContent
State::sign(const Sender& sender,
            Inner&& inner_content,
            const bytes& authenticated_data,
            bool encrypt) const
{
  auto content = GroupContent{ _group_id,
                               _epoch,
                               sender,
                               authenticated_data,
                               { std::forward<Inner>(inner_content) } };

  auto wire_format = (encrypt) ? WireFormat::mls_private_message
                               : WireFormat::mls_public_message;

  auto content_auth = AuthenticatedContent::sign(
    wire_format, std::move(content), _suite, _identity_priv, group_context());

  return content_auth;
}

MLSMessage
State::protect(AuthenticatedContent&& content_auth, size_t padding_size)
{
  switch (content_auth.wire_format) {
    case WireFormat::mls_public_message:
      return PublicMessage::protect(std::move(content_auth),
                                    _suite,
                                    _key_schedule.membership_key,
                                    group_context());

    case WireFormat::mls_private_message:
      return PrivateMessage::protect(std::move(content_auth),
                                     _suite,
                                     _keys,
                                     _key_schedule.sender_data_secret,
                                     padding_size);

    default:
      throw InvalidParameterError("Malformed AuthenticatedContent");
  }
}

ValidatedContent
State::unwrap(const MLSMessage& msg)
{
  if (msg.version != ProtocolVersion::mls10) {
    throw InvalidParameterError("Unsupported version");
  }

  const auto unprotect = overloaded{
    [&](const PublicMessage& pt) -> ValidatedContent {
      if (pt.get_group_id() != _group_id) {
        throw ProtocolError("PublicMessage not for this group");
      }

      if (pt.get_epoch() != _epoch) {
        throw ProtocolError("PublicMessage not for this epoch");
      }

      auto maybe_content_auth =
        pt.unprotect(_suite, _key_schedule.membership_key, group_context());
      if (!maybe_content_auth) {
        throw ProtocolError("Membership tag failed to verify");
      }
      return opt::get(maybe_content_auth);
    },

    [&](const PrivateMessage& ct) -> ValidatedContent {
      if (ct.get_group_id() != _group_id) {
        throw ProtocolError("PrivateMessage not for this group");
      }

      if (ct.get_epoch() != _epoch) {
        throw ProtocolError("PrivateMessage not for this epoch");
      }

      auto maybe_content_auth =
        ct.unprotect(_suite, _keys, _key_schedule.sender_data_secret);
      if (!maybe_content_auth) {
        throw ProtocolError("PrivateMessage decryption failure");
      }
      return opt::get(maybe_content_auth);
    },

    [](const auto& /* unused */) -> ValidatedContent {
      throw ProtocolError("Invalid wire format");
    },
  };

  auto val_content = var::visit(unprotect, msg.message);
  if (!verify(val_content.content_auth)) {
    throw InvalidParameterError("Message signature failed to verify");
  }

  return val_content;
}

Proposal
State::add_proposal(const KeyPackage& key_package) const
{
  // Check that the key package is validly signed
  if (!key_package.verify()) {
    throw InvalidParameterError("Invalid signature on key package");
  }

  // Check that the group's basic properties are supported
  auto now = seconds_since_epoch();
  if (!key_package.leaf_node.verify_expiry(now)) {
    throw InvalidParameterError("Expired key package");
  }

  // Check that the group's extensions are supported
  if (!key_package.leaf_node.verify_extension_support(_extensions)) {
    throw InvalidParameterError(
      "Key package does not support group's extensions");
  }

  return { Add{ key_package } };
}

Proposal
State::update_proposal(HPKEPrivateKey leaf_priv, const LeafNodeOptions& opts)
{
  if (_cached_update) {
    throw ProtocolError("Only one update may be generated per epoch");
  }

  auto leaf = opt::get(_tree.leaf_node(_index));

  auto new_leaf = leaf.for_update(
    _suite, _group_id, _index, leaf_priv.public_key, opts, _identity_priv);

  auto update = Update{ new_leaf };
  _cached_update = CachedUpdate{ std::move(leaf_priv), update };
  return { update };
}

Proposal
State::remove_proposal(RosterIndex index) const
{
  return remove_proposal(leaf_for_roster_entry(index));
}

Proposal
State::remove_proposal(LeafIndex removed) const
{
  if (!_tree.has_leaf(removed)) {
    throw InvalidParameterError("Remove on blank leaf");
  }

  return { Remove{ removed } };
}

Proposal
State::group_context_extensions_proposal(ExtensionList exts) const
{
  if (!extensions_supported(exts)) {
    throw InvalidParameterError("Unsupported extensions");
  }

  return { GroupContextExtensions{ std::move(exts) } };
}

Proposal
State::pre_shared_key_proposal(const bytes& external_psk_id) const
{
  if (_external_psks.count(external_psk_id) == 0) {
    throw InvalidParameterError("Unknown PSK");
  }

  auto psk_id = PreSharedKeyID{
    { ExternalPSK{ external_psk_id } },
    random_bytes(_suite.secret_size()),
  };
  return { PreSharedKey{ psk_id } };
}

Proposal
State::pre_shared_key_proposal(const bytes& group_id, epoch_t epoch) const
{
  if (epoch != _epoch && _resumption_psks.count({ group_id, epoch }) == 0) {
    throw InvalidParameterError("Unknown PSK");
  }

  auto psk_id = PreSharedKeyID{
    { ResumptionPSK{ ResumptionPSKUsage::application, group_id, epoch } },
    random_bytes(_suite.secret_size()),
  };
  return { PreSharedKey{ psk_id } };
}

Proposal
State::reinit_proposal(bytes group_id,
                       ProtocolVersion version,
                       CipherSuite cipher_suite,
                       ExtensionList extensions)
{
  return { ReInit{
    std::move(group_id), version, cipher_suite, std::move(extensions) } };
}

MLSMessage
State::add(const KeyPackage& key_package, const MessageOpts& msg_opts)
{
  return protect_full(add_proposal(key_package), msg_opts);
}

MLSMessage
State::update(HPKEPrivateKey leaf_priv,
              const LeafNodeOptions& opts,
              const MessageOpts& msg_opts)
{
  return protect_full(update_proposal(std::move(leaf_priv), opts), msg_opts);
}

MLSMessage
State::remove(RosterIndex index, const MessageOpts& msg_opts)
{
  return protect_full(remove_proposal(index), msg_opts);
}

MLSMessage
State::remove(LeafIndex removed, const MessageOpts& msg_opts)
{
  return protect_full(remove_proposal(removed), msg_opts);
}

MLSMessage
State::group_context_extensions(ExtensionList exts, const MessageOpts& msg_opts)
{
  return protect_full(group_context_extensions_proposal(std::move(exts)),
                      msg_opts);
}

MLSMessage
State::pre_shared_key(const bytes& external_psk_id, const MessageOpts& msg_opts)
{
  return protect_full(pre_shared_key_proposal(external_psk_id), msg_opts);
}

MLSMessage
State::pre_shared_key(const bytes& group_id,
                      epoch_t epoch,
                      const MessageOpts& msg_opts)
{
  return protect_full(pre_shared_key_proposal(group_id, epoch), msg_opts);
}

MLSMessage
State::reinit(bytes group_id,
              ProtocolVersion version,
              CipherSuite cipher_suite,
              ExtensionList extensions,
              const MessageOpts& msg_opts)
{
  return protect_full(
    reinit_proposal(
      std::move(group_id), version, cipher_suite, std::move(extensions)),
    msg_opts);
}

std::tuple<MLSMessage, Welcome, State>
State::commit(const bytes& leaf_secret,
              const std::optional<CommitOpts>& opts,
              const MessageOpts& msg_opts)
{
  return commit(leaf_secret, opts, msg_opts, NormalCommitParams{});
}

std::tuple<MLSMessage, Welcome, State>
State::commit(const bytes& leaf_secret,
              const std::optional<CommitOpts>& opts,
              const MessageOpts& msg_opts,
              CommitParams params)
{
  // Construct a commit from cached proposals
  // TODO(rlb) ignore some proposals:
  // * Update after Update
  // * Update after Remove
  // * Remove after Remove
  Commit commit;
  auto joiners = std::vector<KeyPackage>{};
  for (const auto& cached : _pending_proposals) {
    if (var::holds_alternative<Add>(cached.proposal.content)) {
      const auto& add = var::get<Add>(cached.proposal.content);
      joiners.push_back(add.key_package);
    }

    commit.proposals.push_back({ cached.ref });
  }

  // Add the extra proposals to those we had cached
  if (opts) {
    const auto& extra_proposals = opt::get(opts).extra_proposals;
    for (const auto& proposal : extra_proposals) {
      if (var::holds_alternative<Add>(proposal.content)) {
        const auto& add = var::get<Add>(proposal.content);
        joiners.push_back(add.key_package);
      }

      commit.proposals.push_back({ proposal });
    }
  }

  // If this is an external commit, insert an ExternalInit proposal
  auto external_commit = std::optional<ExternalCommitParams>{};
  if (var::holds_alternative<ExternalCommitParams>(params)) {
    external_commit = var::get<ExternalCommitParams>(params);
  }

  auto force_init_secret = std::optional<bytes>{};
  if (external_commit) {
    force_init_secret = opt::get(external_commit).force_init_secret;
  }

  // Apply proposals
  State next = successor();

  const auto proposals = must_resolve(commit.proposals, _index);
  if (!valid(proposals, _index, params)) {
    throw ProtocolError("Invalid proposal list");
  }

  const auto [joiner_locations, psks] = next.apply(proposals);

  if (external_commit) {
    const auto& leaf_node =
      opt::get(external_commit).joiner_key_package.leaf_node;
    next._index = next._tree.add_leaf(leaf_node);
  }

  // If this is an external commit, indicate it in the sender field
  auto sender = Sender{ MemberSender{ _index } };
  if (external_commit) {
    sender = Sender{ NewMemberCommitSender{} };
  }

  // KEM new entropy to the group and the new joiners
  auto commit_secret = _suite.zero();
  auto path_secrets =
    std::vector<std::optional<bytes>>(joiner_locations.size());
  auto force_path = opts && opt::get(opts).force_path;
  if (force_path || path_required(proposals)) {
    auto leaf_node_opts = LeafNodeOptions{};
    if (opts) {
      leaf_node_opts = opt::get(opts).leaf_node_opts;
    }

    auto new_priv = next._tree.update(
      next._index, leaf_secret, next._group_id, _identity_priv, leaf_node_opts);

    auto ctx = tls::marshal(GroupContext{
      next._suite,
      next._group_id,
      next._epoch + 1,
      next._tree.root_hash(),
      next._transcript_hash.confirmed,
      next._extensions,
    });
    auto path = next._tree.encap(new_priv, ctx, joiner_locations);

    next._tree_priv = new_priv;
    commit.path = path;
    commit_secret = new_priv.update_secret;

    for (size_t i = 0; i < joiner_locations.size(); i++) {
      auto [overlap, shared_path_secret, ok] =
        new_priv.shared_path_secret(joiner_locations[i]);
      silence_unused(overlap);
      silence_unused(ok);

      path_secrets[i] = shared_path_secret;
    }
  }

  // Create the Commit message and advance the transcripts / key schedule
  auto commit_content_auth =
    sign(sender, commit, msg_opts.authenticated_data, msg_opts.encrypt);

  next._transcript_hash.update_confirmed(commit_content_auth);
  next._epoch += 1;
  next.update_epoch_secrets(commit_secret, psks, force_init_secret);

  const auto confirmation_tag =
    next._key_schedule.confirmation_tag(next._transcript_hash.confirmed);
  commit_content_auth.set_confirmation_tag(confirmation_tag);

  next._transcript_hash.update_interim(commit_content_auth);

  auto commit_message =
    protect(std::move(commit_content_auth), msg_opts.padding_size);

  // Complete the GroupInfo and form the Welcome
  auto group_info = GroupInfo{
    {
      next._suite,
      next._group_id,
      next._epoch,
      next._tree.root_hash(),
      next._transcript_hash.confirmed,
      next._extensions,
    },
    { /* No other extensions */ },
    { confirmation_tag },
  };
  if (opts && opt::get(opts).inline_tree) {
    group_info.extensions.add(RatchetTreeExtension{ next._tree });
  }
  group_info.sign(next._tree, next._index, next._identity_priv);

  auto welcome =
    Welcome{ _suite, next._key_schedule.joiner_secret, psks, group_info };
  for (size_t i = 0; i < joiners.size(); i++) {
    welcome.encrypt(joiners[i], path_secrets[i]);
  }

  return std::make_tuple(commit_message, welcome, next);
}

///
/// Message handlers
///

GroupContext
State::group_context() const
{
  return GroupContext{
    _suite,
    _group_id,
    _epoch,
    _tree.root_hash(),
    _transcript_hash.confirmed,
    _extensions,
  };
}

std::optional<State>
State::handle(const MLSMessage& msg)
{
  return handle(unwrap(msg), std::nullopt, std::nullopt);
}

std::optional<State>
State::handle(const MLSMessage& msg, std::optional<State> cached_state)
{
  return handle(unwrap(msg), std::move(cached_state), std::nullopt);
}

std::optional<State>
State::handle(const ValidatedContent& content_auth)
{
  return handle(content_auth, std::nullopt, std::nullopt);
}

std::optional<State>
State::handle(const ValidatedContent& content_auth,
              std::optional<State> cached_state)
{
  return handle(content_auth, std::move(cached_state), std::nullopt);
}

std::optional<State>
State::handle(const MLSMessage& msg,
              std::optional<State> cached_state,
              const std::optional<CommitParams>& expected_params)
{
  return handle(unwrap(msg), std::move(cached_state), expected_params);
}

std::optional<State>
State::handle(const ValidatedContent& val_content,
              std::optional<State> cached_state,
              const std::optional<CommitParams>& expected_params)
{
  // Dispatch on content type
  const auto& content_auth = val_content.authenticated_content();
  const auto& content = content_auth.content;
  switch (content.content_type()) {
    // Proposals get queued, do not result in a state transition
    case ContentType::proposal:
      cache_proposal(content_auth);
      return std::nullopt;

    // Commits are handled in the remainder of this method
    case ContentType::commit:
      break;

    // Any other content type in this method is an error
    default:
      throw InvalidParameterError("Invalid content type");
  }

  switch (content.sender.sender_type()) {
    case SenderType::member:
    case SenderType::new_member_commit:
      break;

    default:
      throw ProtocolError("Invalid commit sender type");
  }

  auto sender = std::optional<LeafIndex>();
  if (content.sender.sender_type() == SenderType::member) {
    sender = var::get<MemberSender>(content.sender.sender).sender;
  }

  if (sender == _index) {
    if (cached_state) {
      // Verify that the cached state is a plausible successor to this state
      const auto& next = opt::get(cached_state);
      if (next._group_id != _group_id || next._epoch != _epoch + 1 ||
          next._index != _index) {
        throw InvalidParameterError("Invalid successor state");
      }

      return next;
    }

    throw InvalidParameterError("Handle own commits with caching");
  }

  // Apply the commit
  const auto& commit = var::get<Commit>(content.content);
  const auto proposals = must_resolve(commit.proposals, sender);

  const auto params = infer_commit_type(sender, proposals, expected_params);
  auto external_commit = var::holds_alternative<ExternalCommitParams>(params);

  // Check that a path is present when required
  if (path_required(proposals) && !commit.path) {
    throw ProtocolError("Path required but not present");
  }

  // Apply the proposals
  auto next = successor();
  auto [joiner_locations, psks] = next.apply(proposals);

  // If this is an external commit, add the joiner to the tree and note the
  // location where they were added.  Also, compute the "externally forced"
  // value that we will use for the init_secret (as opposed to the init_secret
  // from the key schedule).
  auto force_init_secret = std::optional<bytes>{};
  auto sender_location = LeafIndex{ 0 };
  if (!external_commit) {
    sender_location = opt::get(sender);
  } else {
    // Find where the joiner will be added
    sender_location = next._tree.allocate_leaf();

    // Extract the forced init secret
    auto kem_output = commit.valid_external();
    if (!kem_output) {
      throw ProtocolError("Invalid external commit");
    }

    force_init_secret =
      _key_schedule.receive_external_init(opt::get(kem_output));
  }

  // Decapsulate and apply the UpdatePath, if provided
  auto commit_secret = _suite.zero();
  if (commit.path) {
    const auto& path = opt::get(commit.path);

    if (!valid(path.leaf_node, LeafNodeSource::commit, sender_location)) {
      throw ProtocolError("Commit path has invalid leaf node");
    }

    if (!next._tree.parent_hash_valid(sender_location, path)) {
      throw ProtocolError("Commit path has invalid parent hash");
    }

    next._tree.merge(sender_location, path);

    auto ctx = tls::marshal(GroupContext{
      next._suite,
      next._group_id,
      next._epoch + 1,
      next._tree.root_hash(),
      next._transcript_hash.confirmed,
      next._extensions,
    });
    next._tree_priv.decap(
      sender_location, next._tree, ctx, path, joiner_locations);

    commit_secret = next._tree_priv.update_secret;
  }

  // Update the transcripts and advance the key schedule
  next._transcript_hash.update(content_auth);
  next._epoch += 1;
  next.update_epoch_secrets(commit_secret, { psks }, force_init_secret);

  // Verify the confirmation MAC
  const auto confirmation_tag =
    next._key_schedule.confirmation_tag(next._transcript_hash.confirmed);
  if (!content_auth.check_confirmation_tag(confirmation_tag)) {
    throw ProtocolError("Confirmation failed to verify");
  }

  return next;
}

///
/// Subgroup branching
///

// Parameters:
// * ctor inputs
// * leaf_secret
// * commit_opts
std::tuple<State, Welcome>
State::create_branch(bytes group_id,
                     HPKEPrivateKey enc_priv,
                     SignaturePrivateKey sig_priv,
                     const LeafNode& leaf_node,
                     ExtensionList extensions,
                     const std::vector<KeyPackage>& key_packages,
                     const bytes& leaf_secret,
                     const CommitOpts& commit_opts) const
{
  // Create new empty group with the appropriate PSK
  auto new_group =
    State{ std::move(group_id), _suite,    std::move(enc_priv),
           std::move(sig_priv), leaf_node, std::move(extensions) };

  new_group.add_resumption_psk(_group_id, _epoch, _key_schedule.resumption_psk);

  // Create Add proposals
  auto proposals = stdx::transform<Proposal>(
    key_packages, [&](const auto& kp) { return new_group.add_proposal(kp); });

  // Create PSK proposal
  proposals.push_back({ PreSharedKey{
    { ResumptionPSK{ ResumptionPSKUsage::branch, _group_id, _epoch },
      random_bytes(_suite.secret_size()) } } });

  // Commit the Add and PSK proposals
  auto opts = CommitOpts{
    proposals,
    commit_opts.inline_tree,
    commit_opts.force_path,
    commit_opts.leaf_node_opts,
  };
  auto [_commit, welcome, state] = new_group.commit(
    leaf_secret, opts, {}, RestartCommitParams{ ResumptionPSKUsage::branch });
  return { state, welcome };
}

State
State::handle_branch(const HPKEPrivateKey& init_priv,
                     HPKEPrivateKey enc_priv,
                     SignaturePrivateKey sig_priv,
                     const KeyPackage& key_package,
                     const Welcome& welcome,
                     const std::optional<TreeKEMPublicKey>& tree) const
{
  auto resumption_psks =
    std::map<EpochRef, bytes>{ { { _group_id, _epoch },
                                 _key_schedule.resumption_psk } };
  auto branch_state = State{
    init_priv,
    std::move(enc_priv),
    std::move(sig_priv),
    key_package,
    welcome,
    tree,
    {},
    resumption_psks,
  };

  if (branch_state._suite != _suite) {
    throw ProtocolError("Attempt to branch with a different ciphersuite");
  }

  if (branch_state._epoch != 1) {
    throw ProtocolError("Branch not done at the beginning of the group");
  }

  return branch_state;
}

State::Tombstone::Tombstone(const State& state_in, ReInit reinit_in)
  : epoch_authenticator(state_in.epoch_authenticator())
  , reinit(std::move(reinit_in))
  , prior_group_id(state_in._group_id)
  , prior_epoch(state_in._epoch)
  , resumption_psk(state_in._key_schedule.resumption_psk)
{
}

std::tuple<State, Welcome>
State::Tombstone::create_welcome(HPKEPrivateKey enc_priv,
                                 SignaturePrivateKey sig_priv,
                                 const LeafNode& leaf_node,
                                 const std::vector<KeyPackage>& key_packages,
                                 const bytes& leaf_secret,
                                 const CommitOpts& commit_opts) const
{
  // Create new empty group with the appropriate PSK
  auto new_group =
    State{ reinit.group_id,     reinit.cipher_suite, std::move(enc_priv),
           std::move(sig_priv), leaf_node,           reinit.extensions };

  new_group.add_resumption_psk(prior_group_id, prior_epoch, resumption_psk);

  // Create Add proposals
  auto proposals = stdx::transform<Proposal>(
    key_packages, [&](const auto& kp) { return new_group.add_proposal(kp); });

  // Create PSK proposal
  proposals.push_back({ PreSharedKey{
    { ResumptionPSK{ ResumptionPSKUsage::reinit, prior_group_id, prior_epoch },
      random_bytes(reinit.cipher_suite.secret_size()) } } });

  // Commit the Add and PSK proposals
  auto opts = CommitOpts{
    proposals,
    commit_opts.inline_tree,
    commit_opts.force_path,
    commit_opts.leaf_node_opts,
  };
  auto [_commit, welcome, state] = new_group.commit(
    leaf_secret, opts, {}, RestartCommitParams{ ResumptionPSKUsage::reinit });
  return { state, welcome };
}

State
State::Tombstone::handle_welcome(
  const HPKEPrivateKey& init_priv,
  HPKEPrivateKey enc_priv,
  SignaturePrivateKey sig_priv,
  const KeyPackage& key_package,
  const Welcome& welcome,
  const std::optional<TreeKEMPublicKey>& tree) const
{
  auto resumption_psks =
    std::map<EpochRef, bytes>{ { { prior_group_id, prior_epoch },
                                 resumption_psk } };
  auto new_state = State{
    init_priv,
    std::move(enc_priv),
    std::move(sig_priv),
    key_package,
    welcome,
    tree,
    {},
    resumption_psks,
  };

  if (new_state._suite != reinit.cipher_suite) {
    throw ProtocolError("Attempt to reinit with the wrong ciphersuite");
  }

  if (new_state._epoch != 1) {
    throw ProtocolError("ReInit not done at the beginning of the group");
  }

  return new_state;
}

std::tuple<State::Tombstone, MLSMessage>
State::reinit_commit(const bytes& leaf_secret,
                     const std::optional<CommitOpts>& opts,
                     const MessageOpts& msg_opts)
{
  // Ensure that either the proposal cache or the inline proposals have a ReInit
  // proposal, and no others.
  auto reinit_proposal = Proposal{};
  if (_pending_proposals.size() == 1) {
    reinit_proposal = _pending_proposals.front().proposal;
  } else if (opts && opt::get(opts).extra_proposals.size() == 1) {
    reinit_proposal = opt::get(opts).extra_proposals.front();
  } else {
    throw ProtocolError("Illegal proposals for reinitialization");
  }

  auto reinit = var::get<ReInit>(reinit_proposal.content);

  // Create the commit
  const auto [commit_msg, welcome, new_state] =
    commit(leaf_secret, opts, msg_opts, ReInitCommitParams{});
  silence_unused(welcome);

  // Create the Tombstone from the terminal state
  return { { new_state, reinit }, commit_msg };
}

State::Tombstone
State::handle_reinit_commit(const MLSMessage& commit_msg)
{
  // Verify the signature and process the commit
  const auto val_content = unwrap(commit_msg);
  const auto& content_auth = val_content.authenticated_content();
  if (!verify(content_auth)) {
    throw InvalidParameterError("Message signature failed to verify");
  }

  auto new_state =
    opt::get(handle(content_auth, std::nullopt, ReInitCommitParams{}));

  // Extract the ReInit and create the Tombstone
  const auto& commit = var::get<Commit>(content_auth.content.content);
  const auto proposals = must_resolve(commit.proposals, std::nullopt);
  if (!valid_reinit(proposals)) {
    throw ProtocolError("Invalid proposals for reinit");
  }

  const auto& reinit_proposal = proposals.front();
  const auto& reinit = var::get<ReInit>(reinit_proposal.proposal.content);
  return Tombstone{ new_state, reinit };
}

///
/// Internals
///

LeafIndex
State::apply(const Add& add)
{
  return _tree.add_leaf(add.key_package.leaf_node);
}

void
State::apply(LeafIndex target, const Update& update)
{
  _tree.update_leaf(target, update.leaf_node);
}

void
State::apply(LeafIndex target,
             const Update& update,
             const HPKEPrivateKey& leaf_priv)
{
  _tree.update_leaf(target, update.leaf_node);
  _tree_priv.set_leaf_priv(leaf_priv);
}

LeafIndex
State::apply(const Remove& remove)
{
  if (!_tree.has_leaf(remove.removed)) {
    throw ProtocolError("Attempt to remove non-member");
  }

  _tree.blank_path(remove.removed);
  return remove.removed;
}

void
State::apply(const GroupContextExtensions& gce)
{
  // TODO(RLB): Update spec to clarify that you MUST verify that the new
  // extensions are compatible with all members.
  if (!extensions_supported(gce.group_context_extensions)) {
    throw ProtocolError("Unsupported extensions in GroupContextExtensions");
  }

  _extensions = gce.group_context_extensions;
}

bool
State::extensions_supported(const ExtensionList& exts) const
{
  return _tree.all_leaves([&](auto /* i */, const auto& leaf_node) {
    return leaf_node.verify_extension_support(exts);
  });
}

void
State::cache_proposal(AuthenticatedContent content_auth)
{
  auto ref = _suite.ref(content_auth);
  if (stdx::any_of(_pending_proposals,
                   [&](const auto& cached) { return cached.ref == ref; })) {
    return;
  }

  auto sender_location = std::optional<LeafIndex>();
  if (content_auth.content.sender.sender_type() == SenderType::member) {
    const auto& sender = content_auth.content.sender.sender;
    sender_location = var::get<MemberSender>(sender).sender;
  }

  const auto& proposal = var::get<Proposal>(content_auth.content.content);

  if (content_auth.content.sender.sender_type() == SenderType::external &&
      !valid_external_proposal_type(proposal.proposal_type())) {
    throw ProtocolError("Invalid external proposal");
  }

  if (!valid(sender_location, proposal)) {
    throw ProtocolError("Invalid proposal");
  }

  _pending_proposals.push_back({
    _suite.ref(content_auth),
    proposal,
    sender_location,
  });
}

std::optional<State::CachedProposal>
State::resolve(const ProposalOrRef& id,
               std::optional<LeafIndex> sender_index) const
{
  if (var::holds_alternative<Proposal>(id.content)) {
    return CachedProposal{
      {},
      var::get<Proposal>(id.content),
      sender_index,
    };
  }

  const auto& ref = var::get<ProposalRef>(id.content);
  for (const auto& cached : _pending_proposals) {
    if (cached.ref == ref) {
      return cached;
    }
  }

  return std::nullopt;
}

std::vector<State::CachedProposal>
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
State::must_resolve(const std::vector<ProposalOrRef>& ids,
                    std::optional<LeafIndex> sender_index) const
{
  auto must_resolve = [&](const auto& id) {
    return opt::get(resolve(id, sender_index));
  };
  return stdx::transform<CachedProposal>(ids, must_resolve);
}

std::vector<PSKWithSecret>
State::resolve(const std::vector<PreSharedKeyID>& psks) const
{
  return stdx::transform<PSKWithSecret>(psks, [&](const auto& psk_id) {
    auto get_secret = overloaded{
      [&](const ExternalPSK& ext_psk) {
        if (_external_psks.count(ext_psk.psk_id) == 0) {
          throw ProtocolError("Unknown external PSK");
        }

        return _external_psks.at(ext_psk.psk_id);
      },

      [&](const ResumptionPSK& res_psk) {
        if (res_psk.psk_epoch == _epoch) {
          return _key_schedule.resumption_psk;
        }

        auto key = std::make_tuple(res_psk.psk_group_id, res_psk.psk_epoch);
        if (_resumption_psks.count(key) == 0) {
          throw ProtocolError("Unknown Resumption PSK");
        }

        return _resumption_psks.at(key);
      },
    };

    auto secret = var::visit(get_secret, psk_id.content);
    return PSKWithSecret{ psk_id, secret };
  });
}

std::vector<LeafIndex>
State::apply(const std::vector<CachedProposal>& proposals,
             Proposal::Type required_type)
{
  auto locations = std::vector<LeafIndex>{};
  for (const auto& cached : proposals) {
    auto proposal_type = cached.proposal.proposal_type();
    if (proposal_type != required_type) {
      continue;
    }

    switch (proposal_type) {
      case ProposalType::add: {
        locations.push_back(apply(var::get<Add>(cached.proposal.content)));
        break;
      }

      case ProposalType::update: {
        const auto& update = var::get<Update>(cached.proposal.content);

        if (!cached.sender) {
          throw ProtocolError("Update without target leaf");
        }

        auto target = opt::get(cached.sender);
        if (target != _index) {
          apply(target, update);
          break;
        }

        if (!_cached_update) {
          throw ProtocolError("Self-update with no cached secret");
        }

        const auto& cached_update = opt::get(_cached_update);
        if (update != cached_update.proposal) {
          throw ProtocolError("Self-update does not match cached data");
        }

        apply(target, update, cached_update.update_priv);
        locations.push_back(target);
        break;
      }

      case ProposalType::remove: {
        const auto& remove = var::get<Remove>(cached.proposal.content);
        locations.push_back(apply(remove));
        break;
      }

      case ProposalType::group_context_extensions: {
        const auto& gce =
          var::get<GroupContextExtensions>(cached.proposal.content);
        apply(gce);
        break;
      }

      default:
        throw ProtocolError("Unsupported proposal type");
    }
  }

  // The cached update needs to be reset after applying proposals, so that it is
  // in a clean state for the next epoch.
  _cached_update.reset();

  return locations;
}

std::tuple<std::vector<LeafIndex>, std::vector<PSKWithSecret>>
State::apply(const std::vector<CachedProposal>& proposals)
{
  apply(proposals, ProposalType::update);
  apply(proposals, ProposalType::remove);
  auto joiner_locations = apply(proposals, ProposalType::add);
  apply(proposals, ProposalType::group_context_extensions);

  // Extract the PSK proposals and look up the secrets
  // TODO(RLB): Factor this out, and also factor the above methods into
  // apply_update, apply_remove, etc.
  auto psk_ids = std::vector<PreSharedKeyID>{};
  for (const auto& cached : proposals) {
    if (cached.proposal.proposal_type() != ProposalType::psk) {
      continue;
    }

    const auto& proposal = var::get<PreSharedKey>(cached.proposal.content);
    psk_ids.push_back(proposal.psk);
  }
  auto psks = resolve(psk_ids);

  _tree.truncate();
  _tree_priv.truncate(_tree.size);
  _tree.set_hash_all();
  return { joiner_locations, psks };
}

///
/// Message protection
///

MLSMessage
State::protect(const bytes& authenticated_data,
               const bytes& pt,
               size_t padding_size)
{
  auto msg_opts = MessageOpts{ true, authenticated_data, padding_size };
  return protect_full(ApplicationData{ pt }, msg_opts);
}

std::tuple<bytes, bytes>
State::unprotect(const MLSMessage& ct)
{
  const auto val_content = unwrap(ct);
  const auto& content_auth = val_content.authenticated_content();

  if (!verify(content_auth)) {
    throw InvalidParameterError("Message signature failed to verify");
  }

  if (content_auth.content.content_type() != ContentType::application) {
    throw ProtocolError("Unprotect of handshake message");
  }

  if (content_auth.wire_format != WireFormat::mls_private_message) {
    throw ProtocolError("Application data not sent as PrivateMessage");
  }

  return {
    content_auth.content.authenticated_data,
    var::get<ApplicationData>(content_auth.content.content).data,
  };
}

///
/// Properties of a proposal list
///

bool
State::valid(const LeafNode& leaf_node,
             LeafNodeSource required_source,
             std::optional<LeafIndex> index) const
{
  // Verify that the credential in the LeafNode is valid as described in Section
  // 5.3.1.
  // XXX(RLB) N/A, no credential validation in the library right now

  // Verify the leaf_node_source field:
  const auto correct_source = (leaf_node.source() == required_source);

  // Verify that the signature on the LeafNode is valid using signature_key.
  auto binding = std::optional<LeafNode::MemberBinding>{};
  switch (required_source) {
    case LeafNodeSource::commit:
    case LeafNodeSource::update:
      binding = LeafNode::MemberBinding{ _group_id, opt::get(index) };
      break;

    default:
      // Nothing to do
      break;
  }

  const auto signature_valid = leaf_node.verify(_suite, binding);

  // Verify that the LeafNode is compatible with the group's parameters. If the
  // GroupContext has a required_capabilities extension, then the required
  // extensions, proposals, and credential types MUST be listed in the
  // LeafNode's capabilities field.
  const auto supports_group_extensions =
    leaf_node.verify_extension_support(_extensions);

  // TODO(RLB) Verify the lifetime field

  // Verify that the credential type is supported by all members of the group,
  // as specified by the capabilities field of each member's LeafNode, and that
  // the capabilities field of this LeafNode indicates support for all the
  // credential types currently in use by other members.
  //
  // Verify that the following fields are unique among the members of the group:
  //   signature_key
  //   encryption_key
  //
  // Note: Uniqueness of signature and encryption keys is assured by the
  // tree operations (add/update), so we do not need to verify those here.
  const auto mutual_credential_support =
    _tree.all_leaves([&](auto /* i */, const auto& leaf) {
      return leaf.capabilities.credential_supported(leaf_node.credential) &&
             leaf_node.capabilities.credential_supported(leaf.credential);
    });

  // Verify that the extensions in the LeafNode are supported by checking that
  // the ID for each extension in the extensions field is listed in the
  // capabilities.extensions field of the LeafNode.
  auto supports_own_extensions =
    leaf_node.verify_extension_support(leaf_node.extensions);

  return (signature_valid && supports_group_extensions && correct_source &&
          mutual_credential_support && supports_own_extensions);
}

bool
State::valid(const KeyPackage& key_package) const
{
  // Verify that the ciphersuite and protocol version of the KeyPackage match
  // those in the GroupContext.
  const auto correct_ciphersuite = (key_package.cipher_suite == _suite);

  // Verify that the signature on the KeyPackage is valid using the public key
  // in leaf_node.credential.
  const auto valid_signature = key_package.verify();

  // Verify that the leaf_node of the KeyPackage is valid for a KeyPackage
  // according to Section 7.3.
  const auto leaf_node_valid =
    valid(key_package.leaf_node, LeafNodeSource::key_package, std::nullopt);

  // Verify that the value of leaf_node.encryption_key is different from the
  // value of the init_key field.
  const auto distinct_keys =
    (key_package.init_key != key_package.leaf_node.encryption_key);

  return (correct_ciphersuite && valid_signature && leaf_node_valid &&
          distinct_keys);
}

bool
State::valid(const Add& add) const
{
  return valid(add.key_package);
}

bool
State::valid(LeafIndex sender, const Update& update) const
{
  const auto maybe_leaf = _tree.leaf_node(sender);
  if (!maybe_leaf) {
    return false;
  }

  return valid(update.leaf_node, LeafNodeSource::update, sender);
}

bool
State::valid(const Remove& remove) const
{
  // We mark self-removes invalid here even though a resync Commit will
  // sometimes cause them.  This is OK because this method is only called from
  // the normal proposal list validation method, not the external commit one.
  auto in_tree = remove.removed < _tree.size && _tree.has_leaf(remove.removed);
  auto not_me = remove.removed != _index;
  return in_tree && not_me;
}

bool
State::valid(const PreSharedKey& psk) const
{
  // External PSKs are allowed if we have the corresponding secret
  if (var::holds_alternative<ExternalPSK>(psk.psk.content)) {
    const auto& ext_psk = var::get<ExternalPSK>(psk.psk.content);
    return _external_psks.count(ext_psk.psk_id) > 0;
  }

  // Resumption PSKs are allowed only with usage 'application', and only if we
  // have the corresponding secret.
  if (var::holds_alternative<ResumptionPSK>(psk.psk.content)) {
    const auto& res_psk = var::get<ResumptionPSK>(psk.psk.content);
    if (res_psk.usage != ResumptionPSKUsage::application) {
      return false;
    }

    const auto key = std::make_tuple(res_psk.psk_group_id, res_psk.psk_epoch);
    return res_psk.psk_epoch == _epoch || _resumption_psks.count(key) > 0;
  }

  return false;
}

bool
State::valid(const ReInit& reinit)
{
  // Check that the version and CipherSuite are ones we support
  auto supported_version = (reinit.version == ProtocolVersion::mls10);
  auto supported_suite =
    stdx::contains(all_supported_suites, reinit.cipher_suite.cipher_suite());

  return supported_version && supported_suite;
}

bool
State::valid(const ExternalInit& external_init) const
{
  return external_init.kem_output.size() == _suite.hpke().kem.enc_size;
}

bool
State::valid(const GroupContextExtensions& gce) const
{
  return extensions_supported(gce.group_context_extensions);
}

bool
State::valid(std::optional<LeafIndex> sender, const Proposal& proposal) const
{
  const auto specifically_valid = overloaded{
    [&](const Update& update) { return valid(opt::get(sender), update); },
    [&](const auto& proposal) { return valid(proposal); },
  };
  return var::visit(specifically_valid, proposal.content);
}

bool
State::valid(const std::vector<CachedProposal>& proposals,
             LeafIndex commit_sender,
             const CommitParams& params) const
{
  auto specifically = overloaded{
    [&](const NormalCommitParams& /* unused */) {
      return valid_normal(proposals, commit_sender);
    },
    [&](const ExternalCommitParams& /* unused */) {
      return valid_external(proposals);
    },
    [&](const RestartCommitParams& params) {
      return valid_restart(proposals, params.allowed_usage);
    },
    [&](const ReInitCommitParams& /* unused */) {
      return valid_reinit(proposals);
    },
  };

  return var::visit(specifically, params);
}

bool
// NB(RLB): clang-tidy thinks this can be static, but it can't.
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
State::valid_normal(const std::vector<CachedProposal>& proposals,
                    LeafIndex commit_sender) const
{
  // It contains an individual proposal that is invalid as specified in Section
  // 12.1.
  const auto has_invalid_proposal =
    stdx::any_of(proposals, [&](const auto& cached) {
      return !valid(cached.sender, cached.proposal);
    });

  // It contains an Update proposal generated by the committer.
  const auto has_self_update = stdx::any_of(proposals, [&](const auto& cached) {
    return cached.proposal.proposal_type() == ProposalType::update &&
           cached.sender == commit_sender;
  });

  // It contains a Remove proposal that removes the committer.
  const auto has_self_remove = stdx::any_of(proposals, [&](const auto& cached) {
    return cached.proposal.proposal_type() == ProposalType::remove &&
           var::get<Remove>(cached.proposal.content).removed == commit_sender;
  });

  // It contains multiple Update and/or Remove proposals that apply to the same
  // leaf. If the committer has received multiple such proposals they SHOULD
  // prefer any Remove received, or the most recent Update if there are no
  // Removes.
  auto updated_or_removed = std::set<LeafIndex>{};
  const auto has_dup_update_remove =
    stdx::any_of(proposals, [&](const auto& cached) {
      auto index = LeafIndex{ 0 };
      switch (cached.proposal.proposal_type()) {
        case ProposalType::update:
          index = opt::get(cached.sender);
          break;

        case ProposalType::remove:
          index = var::get<Remove>(cached.proposal.content).removed;
          break;

        default:
          return false;
      }

      if (stdx::contains(updated_or_removed, index)) {
        return true;
      }

      updated_or_removed.insert(index);
      return false;
    });

  // It contains multiple Add proposals that contain KeyPackages that represent
  // the same client according to the application (for example, identical
  // signature keys).
  auto signature_keys = std::vector<SignaturePublicKey>{};
  const auto has_dup_signature_key =
    stdx::any_of(proposals, [&](const auto& cached) {
      if (cached.proposal.proposal_type() != ProposalType::add) {
        return false;
      }

      auto key_package = var::get<Add>(cached.proposal.content).key_package;
      auto signature_key = key_package.leaf_node.signature_key;
      if (stdx::contains(signature_keys, signature_key)) {
        return true;
      }

      signature_keys.push_back(signature_key);
      return false;
    });

  // It contains an Add proposal with a KeyPackage that represents a client
  // already in the group according to the application, unless there is a Remove
  // proposal in the list removing the matching client from the group.
  // TODO(RLB)

  // It contains multiple PreSharedKey proposals that reference the same
  // PreSharedKeyID.
  auto psk_ids = std::vector<PreSharedKeyID>{};
  const auto has_dup_psk_id = stdx::any_of(proposals, [&](const auto& cached) {
    if (cached.proposal.proposal_type() != ProposalType::psk) {
      return false;
    }

    auto psk_id = var::get<PreSharedKey>(cached.proposal.content).psk;
    if (stdx::contains(psk_ids, psk_id)) {
      return true;
    }

    psk_ids.push_back(psk_id);
    return false;
  });

  // It contains multiple GroupContextExtensions proposals.
  const auto gce_count = stdx::count_if(proposals, [](const auto& cached) {
    return cached.proposal.proposal_type() ==
           ProposalType::group_context_extensions;
  });
  const auto has_multiple_gce = (gce_count > 1);

  // It contains a ReInit proposal together with any other proposal. If the
  // committer has received other proposals during the epoch, they SHOULD prefer
  // them over the ReInit proposal, allowing the ReInit to be resent and applied
  // in a subsequent epoch.
  const auto has_reinit = stdx::any_of(proposals, [](const auto& cached) {
    return cached.proposal.proposal_type() == ProposalType::reinit;
  });

  // It contains an ExternalInit proposal.
  const auto has_external_init =
    stdx::any_of(proposals, [](const auto& cached) {
      return cached.proposal.proposal_type() == ProposalType::external_init;
    });

  // It contains a proposal with a non-default proposal type that is not
  // supported by some members of the group that will process the Commit (i.e.,
  // members being added or removed by the Commit do not need to support the
  // proposal type).
  // XXX(RLB): N/A, no non-default proposal types

  // After processing the commit the ratchet tree is invalid, in particular, if
  // it contains any leaf node that is invalid according to Section 7.3.
  //
  // NB(RLB): Leaf nodes are already checked in the individual proposal check at
  // the top.  So the focus here is key uniqueness. We check this by checking
  // uniqueness of encryption keys across the Adds and Updates in this list of
  // proposals.  The keys have already been checked to be distinct from any keys
  // already in the tree.
  auto enc_keys = std::vector<HPKEPublicKey>{};
  const auto has_dup_enc_key = stdx::any_of(proposals, [&](const auto& cached) {
    const auto get_enc_key =
      overloaded{ [](const Add& add) -> std::optional<HPKEPublicKey> {
                   return add.key_package.leaf_node.encryption_key;
                 },
                  [](const Update& update) -> std::optional<HPKEPublicKey> {
                    return update.leaf_node.encryption_key;
                  },

                  [](const auto& /* default */)
                    -> std::optional<HPKEPublicKey> { return std::nullopt; } };
    auto maybe_enc_key = var::visit(get_enc_key, cached.proposal.content);
    if (!maybe_enc_key) {
      return false;
    }

    const auto& enc_key = opt::get(maybe_enc_key);
    if (stdx::contains(enc_keys, enc_key)) {
      return true;
    }

    enc_keys.push_back(enc_key);
    return false;
  });

  return !(has_invalid_proposal || has_self_update || has_self_remove ||
           has_dup_update_remove || has_dup_signature_key || has_dup_psk_id ||
           has_multiple_gce || has_reinit || has_external_init ||
           has_dup_enc_key);
}

bool
State::valid_reinit(const std::vector<CachedProposal>& proposals)
{
  // Check that the list contains a ReInit proposal
  const auto has_reinit = stdx::any_of(proposals, [](const auto& cached) {
    return cached.proposal.proposal_type() == ProposalType::reinit;
  });

  // Check whether the list contains any disallowed proposals
  const auto has_disallowed = stdx::any_of(proposals, [](const auto& cached) {
    return cached.proposal.proposal_type() != ProposalType::reinit;
  });

  return has_reinit && !has_disallowed;
}

bool
State::valid_restart(const std::vector<CachedProposal>& proposals,
                     ResumptionPSKUsage allowed_usage)
{
  // Check that the list has exactly one resumption PSK proposal with the
  // allowed usage and any other PSKs are external
  auto found_allowed = false;
  const auto acceptable_psks = stdx::all_of(proposals, [&](const auto& cached) {
    if (cached.proposal.proposal_type() != ProposalType::psk) {
      return true;
    }

    const auto& psk = var::get<PreSharedKey>(cached.proposal.content);
    if (var::holds_alternative<ExternalPSK>(psk.psk.content)) {
      return true;
    }

    const auto& res_psk = var::get<ResumptionPSK>(psk.psk.content);
    const auto allowed = res_psk.usage == allowed_usage;
    if (found_allowed && allowed) {
      return false;
    }

    found_allowed = found_allowed || allowed;
    return true;
  });

  return acceptable_psks && found_allowed;
}

bool
State::valid_external_proposal_type(const Proposal::Type proposal_type)
{
  switch (proposal_type) {
    case ProposalType::add:
    case ProposalType::remove:
    case ProposalType::psk:
    case ProposalType::reinit:
    case ProposalType::group_context_extensions:
      return true;

    default:
      return false;
  }
}

bool
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
State::valid_external(const std::vector<CachedProposal>& proposals) const
{
  // Exactly one ExternalInit
  auto ext_init_count = stdx::count_if(proposals, [](const auto& cached) {
    return cached.proposal.proposal_type() == ProposalType::external_init;
  });
  auto one_ext_init = (ext_init_count == 1);

  // At most one Remove proposal, with which the joiner removes an old version
  // of themselves. If a Remove proposal is present, then the LeafNode in the
  // path field of the external commit MUST meet the same criteria as would the
  // LeafNode in an Update for the removed leaf (see Section 12.1.2). In
  // particular, the credential in the LeafNode MUST present a set of
  // identifiers that is acceptable to the application for the removed
  // participant.
  // TODO(RLB) Verify that Remove is properly formed
  auto remove_count = stdx::count_if(proposals, [](const auto& cached) {
    return cached.proposal.proposal_type() == ProposalType::remove;
  });
  auto no_dup_remove = (remove_count <= 1);

  // Zero or more PreSharedKey proposals.
  // No other proposals.
  auto no_disallowed = stdx::all_of(proposals, [&](const auto& cached) {
    switch (cached.proposal.proposal_type()) {
      case ProposalType::external_init:
      case ProposalType::remove:
        return true;

      case ProposalType::psk:
        return valid(var::get<PreSharedKey>(cached.proposal.content));

      default:
        return false;
    }
  });

  return one_ext_init && no_dup_remove && no_disallowed;
}

State::CommitParams
State::infer_commit_type(
  const std::optional<LeafIndex>& sender,
  const std::vector<CachedProposal>& proposals,
  const std::optional<CommitParams>& expected_params) const
{
  // If an expected type was provided, validate against it
  if (expected_params) {
    const auto& expected = opt::get(expected_params);

    auto specifically = overloaded{
      [&](const NormalCommitParams& /* unused */) {
        return sender && valid_normal(proposals, opt::get(sender));
      },
      [&](const ExternalCommitParams& /* unused */) {
        return !sender && valid_external(proposals);
      },
      [&](const RestartCommitParams& params) {
        return sender && valid_restart(proposals, params.allowed_usage);
      },
      [&](const ReInitCommitParams& /* unused */) {
        return sender && valid_reinit(proposals);
      },
    };

    if (!var::visit(specifically, expected)) {
      throw ProtocolError("Invalid proposal list");
    }

    return expected;
  }

  // Otherwise, check to see if this is a valid external or normal commit
  if (!sender && valid_external(proposals)) {
    return ExternalCommitParams{};
  }

  if (sender && valid_normal(proposals, opt::get(sender))) {
    return NormalCommitParams{};
  }

  throw ProtocolError("Invalid proposal list");
}

bool
State::path_required(const std::vector<CachedProposal>& proposals)
{
  static const auto path_required_types = std::set<Proposal::Type>{
    ProposalType::update,
    ProposalType::remove,
    ProposalType::external_init,
    ProposalType::group_context_extensions,
  };

  if (proposals.empty()) {
    return true;
  }

  return stdx::any_of(proposals, [](const auto& cp) {
    return path_required_types.count(cp.proposal.proposal_type()) != 0;
  });
}

///
/// Inner logic and convenience functions
///

bool
operator==(const State& lhs, const State& rhs)
{
  auto suite = (lhs._suite == rhs._suite);
  auto group_id = (lhs._group_id == rhs._group_id);
  auto epoch = (lhs._epoch == rhs._epoch);
  auto tree = (lhs._tree == rhs._tree);
  auto transcript_hash = (lhs._transcript_hash == rhs._transcript_hash);
  auto key_schedule = (lhs._key_schedule == rhs._key_schedule);
  auto extensions = (lhs._extensions == rhs._extensions);

  return suite && group_id && epoch && tree && transcript_hash &&
         key_schedule && extensions;
}

bool
operator!=(const State& lhs, const State& rhs)
{
  return !(lhs == rhs);
}

void
State::update_epoch_secrets(const bytes& commit_secret,
                            const std::vector<PSKWithSecret>& psks,
                            const std::optional<bytes>& force_init_secret)
{
  auto ctx = tls::marshal(GroupContext{
    _suite,
    _group_id,
    _epoch,
    _tree.root_hash(),
    _transcript_hash.confirmed,
    _extensions,
  });
  _key_schedule =
    _key_schedule.next(commit_secret, psks, force_init_secret, ctx);
  _keys = _key_schedule.encryption_keys(_tree.size);
}

///
/// Message encryption and decryption
///
bool
State::verify_internal(const AuthenticatedContent& content_auth) const
{
  const auto& sender =
    var::get<MemberSender>(content_auth.content.sender.sender).sender;
  auto maybe_leaf = _tree.leaf_node(sender);
  if (!maybe_leaf) {
    throw InvalidParameterError("Signature from blank node");
  }

  const auto& pub = opt::get(maybe_leaf).signature_key;
  return content_auth.verify(_suite, pub, group_context());
}

bool
State::verify_external(const AuthenticatedContent& content_auth) const
{
  const auto& ext_sender =
    var::get<ExternalSenderIndex>(content_auth.content.sender.sender);
  const auto senders_ext = _extensions.find<ExternalSendersExtension>();
  const auto& senders = opt::get(senders_ext).senders;
  const auto& pub = senders.at(ext_sender.sender_index).signature_key;
  return content_auth.verify(_suite, pub, group_context());
}

bool
State::verify_new_member_proposal(
  const AuthenticatedContent& content_auth) const
{
  const auto& proposal = var::get<Proposal>(content_auth.content.content);
  const auto& add = var::get<Add>(proposal.content);
  const auto& pub = add.key_package.leaf_node.signature_key;
  return content_auth.verify(_suite, pub, group_context());
}

bool
State::verify_new_member_commit(const AuthenticatedContent& content_auth) const
{
  const auto& commit = var::get<Commit>(content_auth.content.content);
  const auto& path = opt::get(commit.path);
  const auto& pub = path.leaf_node.signature_key;
  return content_auth.verify(_suite, pub, group_context());
}

bool
State::verify(const AuthenticatedContent& content_auth) const
{
  switch (content_auth.content.sender.sender_type()) {
    case SenderType::member:
      return verify_internal(content_auth);

    case SenderType::external:
      return verify_external(content_auth);

    case SenderType::new_member_proposal:
      return verify_new_member_proposal(content_auth);

    case SenderType::new_member_commit:
      return verify_new_member_commit(content_auth);

    default:
      throw ProtocolError("Invalid sender type");
  }
}

void
State::add_resumption_psk(const bytes& group_id, epoch_t epoch, bytes secret)
{
  _resumption_psks.insert_or_assign({ group_id, epoch }, std::move(secret));
}

void
State::remove_resumption_psk(const bytes& group_id, epoch_t epoch)
{
  _resumption_psks.erase({ group_id, epoch });
}

void
State::add_external_psk(const bytes& id, const bytes& secret)
{
  _external_psks.insert_or_assign(id, secret);
}

void
State::remove_external_psk(const bytes& id)
{
  _external_psks.erase(id);
}

bytes
State::do_export(const std::string& label,
                 const bytes& context,
                 size_t size) const
{
  return _key_schedule.do_export(label, context, size);
}

GroupInfo
State::group_info(bool inline_tree) const
{
  auto group_info = GroupInfo{
    {
      _suite,
      _group_id,
      _epoch,
      _tree.root_hash(),
      _transcript_hash.confirmed,
      _extensions,
    },
    { /* No other extensions */ },
    _key_schedule.confirmation_tag(_transcript_hash.confirmed),
  };

  group_info.extensions.add(
    ExternalPubExtension{ _key_schedule.external_priv.public_key });

  if (inline_tree) {
    group_info.extensions.add(RatchetTreeExtension{ _tree });
  }

  group_info.sign(_tree, _index, _identity_priv);
  return group_info;
}

std::vector<LeafNode>
State::roster() const
{
  auto leaves = std::vector<LeafNode>{};
  leaves.reserve(_tree.size.val);

  _tree.all_leaves([&](auto /* i */, auto leaf) {
    leaves.push_back(leaf);
    return true;
  });

  return leaves;
}

bytes
State::epoch_authenticator() const
{
  return _key_schedule.epoch_authenticator;
}

LeafIndex
State::leaf_for_roster_entry(RosterIndex index) const
{
  auto visited = RosterIndex{ 0 };
  auto found = std::optional<LeafIndex>{};
  _tree.all_leaves([&](auto i, const auto& /* leaf_node */) {
    if (visited == index) {
      found = i;
      return false;
    }

    visited.val += 1;
    return true;
  });

  return opt::get(found);
}

State
State::successor() const
{
  // Copy everything, then clear things that shouldn't be copied
  auto next = *this;
  next._pending_proposals.clear();

  // Copy forward a resumption PSK
  next.add_resumption_psk(_group_id, _epoch, _key_schedule.resumption_psk);

  return next;
}

} // namespace mlspp
