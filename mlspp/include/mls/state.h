#pragma once

#include "mls/crypto.h"
#include "mls/key_schedule.h"
#include "mls/messages.h"
#include "mls/treekem.h"
#include <list>
#include <optional>
#include <vector>

namespace mlspp {

// Index into the session roster
struct RosterIndex : public UInt32
{
  using UInt32::UInt32;
};

struct CommitOpts
{
  std::vector<Proposal> extra_proposals;
  bool inline_tree;
  bool force_path;
  LeafNodeOptions leaf_node_opts;
};

struct MessageOpts
{
  bool encrypt = false;
  bytes authenticated_data;
  size_t padding_size = 0;
};

class State
{
public:
  ///
  /// Constructors
  ///

  // Initialize an empty group
  State(bytes group_id,
        CipherSuite suite,
        HPKEPrivateKey enc_priv,
        SignaturePrivateKey sig_priv,
        const LeafNode& leaf_node,
        ExtensionList extensions);

  // Initialize a group from a Welcome
  State(const HPKEPrivateKey& init_priv,
        HPKEPrivateKey leaf_priv,
        SignaturePrivateKey sig_priv,
        const KeyPackage& key_package,
        const Welcome& welcome,
        const std::optional<TreeKEMPublicKey>& tree,
        std::map<bytes, bytes> psks);

  // Join a group from outside
  // XXX(RLB) To be fully general, we would need a few more options here, e.g.,
  // whether to include PSKs or evict our prior appearance.
  static std::tuple<MLSMessage, State> external_join(
    const bytes& leaf_secret,
    SignaturePrivateKey sig_priv,
    const KeyPackage& key_package,
    const GroupInfo& group_info,
    const std::optional<TreeKEMPublicKey>& tree,
    const MessageOpts& msg_opts,
    std::optional<LeafIndex> remove_prior,
    const std::map<bytes, bytes>& psks);

  // Propose that a new member be added a group
  static MLSMessage new_member_add(const bytes& group_id,
                                   epoch_t epoch,
                                   const KeyPackage& new_member,
                                   const SignaturePrivateKey& sig_priv);

  ///
  /// Message factories
  ///

  Proposal add_proposal(const KeyPackage& key_package) const;
  Proposal update_proposal(HPKEPrivateKey leaf_priv,
                           const LeafNodeOptions& opts);
  Proposal remove_proposal(RosterIndex index) const;
  Proposal remove_proposal(LeafIndex removed) const;
  Proposal group_context_extensions_proposal(ExtensionList exts) const;
  Proposal pre_shared_key_proposal(const bytes& external_psk_id) const;
  Proposal pre_shared_key_proposal(const bytes& group_id, epoch_t epoch) const;
  static Proposal reinit_proposal(bytes group_id,
                                  ProtocolVersion version,
                                  CipherSuite cipher_suite,
                                  ExtensionList extensions);

  MLSMessage add(const KeyPackage& key_package, const MessageOpts& msg_opts);
  MLSMessage update(HPKEPrivateKey leaf_priv,
                    const LeafNodeOptions& opts,
                    const MessageOpts& msg_opts);
  MLSMessage remove(RosterIndex index, const MessageOpts& msg_opts);
  MLSMessage remove(LeafIndex removed, const MessageOpts& msg_opts);
  MLSMessage group_context_extensions(ExtensionList exts,
                                      const MessageOpts& msg_opts);
  MLSMessage pre_shared_key(const bytes& external_psk_id,
                            const MessageOpts& msg_opts);
  MLSMessage pre_shared_key(const bytes& group_id,
                            epoch_t epoch,
                            const MessageOpts& msg_opts);
  MLSMessage reinit(bytes group_id,
                    ProtocolVersion version,
                    CipherSuite cipher_suite,
                    ExtensionList extensions,
                    const MessageOpts& msg_opts);

  std::tuple<MLSMessage, Welcome, State> commit(
    const bytes& leaf_secret,
    const std::optional<CommitOpts>& opts,
    const MessageOpts& msg_opts);

  ///
  /// Generic handshake message handlers
  ///
  std::optional<State> handle(const MLSMessage& msg);
  std::optional<State> handle(const MLSMessage& msg,
                              std::optional<State> cached_state);

  std::optional<State> handle(const ValidatedContent& content_auth);
  std::optional<State> handle(const ValidatedContent& content_auth,
                              std::optional<State> cached_state);

  ///
  /// PSK management
  ///
  void add_resumption_psk(const bytes& group_id, epoch_t epoch, bytes secret);
  void remove_resumption_psk(const bytes& group_id, epoch_t epoch);
  void add_external_psk(const bytes& id, const bytes& secret);
  void remove_external_psk(const bytes& id);

  ///
  /// Accessors
  ///
  const bytes& group_id() const { return _group_id; }
  epoch_t epoch() const { return _epoch; }
  LeafIndex index() const { return _index; }
  CipherSuite cipher_suite() const { return _suite; }
  const ExtensionList& extensions() const { return _extensions; }
  const TreeKEMPublicKey& tree() const { return _tree; }
  const bytes& resumption_psk() const { return _key_schedule.resumption_psk; }

  bytes do_export(const std::string& label,
                  const bytes& context,
                  size_t size) const;
  GroupInfo group_info(bool inline_tree) const;

  // Ordered list of credentials from non-blank leaves
  std::vector<LeafNode> roster() const;

  bytes epoch_authenticator() const;

  ///
  /// Unwrap messages so that applications can inspect them
  ///
  ValidatedContent unwrap(const MLSMessage& msg);

  ///
  /// Application encryption and decryption
  ///
  MLSMessage protect(const bytes& authenticated_data,
                     const bytes& pt,
                     size_t padding_size);
  std::tuple<bytes, bytes> unprotect(const MLSMessage& ct);

  // Assemble a group context for this state
  GroupContext group_context() const;

  // Subgroup branching
  std::tuple<State, Welcome> create_branch(
    bytes group_id,
    HPKEPrivateKey enc_priv,
    SignaturePrivateKey sig_priv,
    const LeafNode& leaf_node,
    ExtensionList extensions,
    const std::vector<KeyPackage>& key_packages,
    const bytes& leaf_secret,
    const CommitOpts& commit_opts) const;
  State handle_branch(const HPKEPrivateKey& init_priv,
                      HPKEPrivateKey enc_priv,
                      SignaturePrivateKey sig_priv,
                      const KeyPackage& key_package,
                      const Welcome& welcome,
                      const std::optional<TreeKEMPublicKey>& tree) const;

  // Reinitialization
  struct Tombstone
  {
    std::tuple<State, Welcome> create_welcome(
      HPKEPrivateKey enc_priv,
      SignaturePrivateKey sig_priv,
      const LeafNode& leaf_node,
      const std::vector<KeyPackage>& key_packages,
      const bytes& leaf_secret,
      const CommitOpts& commit_opts) const;
    State handle_welcome(const HPKEPrivateKey& init_priv,
                         HPKEPrivateKey enc_priv,
                         SignaturePrivateKey sig_priv,
                         const KeyPackage& key_package,
                         const Welcome& welcome,
                         const std::optional<TreeKEMPublicKey>& tree) const;

    TLS_SERIALIZABLE(prior_group_id, prior_epoch, resumption_psk, reinit);

    const bytes epoch_authenticator;
    const ReInit reinit;

  private:
    Tombstone(const State& state_in, ReInit reinit_in);

    bytes prior_group_id;
    epoch_t prior_epoch;
    bytes resumption_psk;

    friend class State;
  };

  std::tuple<Tombstone, MLSMessage> reinit_commit(
    const bytes& leaf_secret,
    const std::optional<CommitOpts>& opts,
    const MessageOpts& msg_opts);
  Tombstone handle_reinit_commit(const MLSMessage& commit);

protected:
  // Shared confirmed state
  // XXX(rlb@ipv.sx): Can these be made const?
  CipherSuite _suite;
  bytes _group_id;
  epoch_t _epoch;
  TreeKEMPublicKey _tree;
  TreeKEMPrivateKey _tree_priv;
  TranscriptHash _transcript_hash;
  ExtensionList _extensions;

  // Shared secret state
  KeyScheduleEpoch _key_schedule;
  GroupKeySource _keys;

  // Per-participant state
  LeafIndex _index;
  SignaturePrivateKey _identity_priv;

  // Storage for PSKs
  std::map<bytes, bytes> _external_psks;

  using EpochRef = std::tuple<bytes, epoch_t>;
  std::map<EpochRef, bytes> _resumption_psks;

  // Cache of Proposals and update secrets
  struct CachedProposal
  {
    ProposalRef ref;
    Proposal proposal;
    std::optional<LeafIndex> sender;
  };
  std::list<CachedProposal> _pending_proposals;

  struct CachedUpdate
  {
    HPKEPrivateKey update_priv;
    Update proposal;
  };
  std::optional<CachedUpdate> _cached_update;

  // Assemble a preliminary, unjoined group state
  State(SignaturePrivateKey sig_priv,
        const GroupInfo& group_info,
        const std::optional<TreeKEMPublicKey>& tree);

  // Assemble a group from a Welcome, allowing for resumption PSKs
  State(const HPKEPrivateKey& init_priv,
        HPKEPrivateKey leaf_priv,
        SignaturePrivateKey sig_priv,
        const KeyPackage& key_package,
        const Welcome& welcome,
        const std::optional<TreeKEMPublicKey>& tree,
        std::map<bytes, bytes> external_psks,
        std::map<EpochRef, bytes> resumption_psks);

  // Import a tree from an externally-provided tree or an extension
  TreeKEMPublicKey import_tree(const bytes& tree_hash,
                               const std::optional<TreeKEMPublicKey>& external,
                               const ExtensionList& extensions);
  bool validate_tree() const;

  // Form a commit, covering all the cases with slightly different validation
  // rules:
  // * Normal
  // * External
  // * Branch
  // * Reinit
  struct NormalCommitParams
  {};

  struct ExternalCommitParams
  {
    KeyPackage joiner_key_package;
    bytes force_init_secret;
  };

  struct RestartCommitParams
  {
    ResumptionPSKUsage allowed_usage;
  };

  struct ReInitCommitParams
  {};

  using CommitParams = var::variant<NormalCommitParams,
                                    ExternalCommitParams,
                                    RestartCommitParams,
                                    ReInitCommitParams>;

  std::tuple<MLSMessage, Welcome, State> commit(
    const bytes& leaf_secret,
    const std::optional<CommitOpts>& opts,
    const MessageOpts& msg_opts,
    CommitParams params);

  std::optional<State> handle(
    const MLSMessage& msg,
    std::optional<State> cached_state,
    const std::optional<CommitParams>& expected_params);
  std::optional<State> handle(
    const ValidatedContent& val_content,
    std::optional<State> cached_state,
    const std::optional<CommitParams>& expected_params);

  // Create an MLSMessage encapsulating some content
  template<typename Inner>
  AuthenticatedContent sign(const Sender& sender,
                            Inner&& content,
                            const bytes& authenticated_data,
                            bool encrypt) const;

  MLSMessage protect(AuthenticatedContent&& content_auth, size_t padding_size);

  template<typename Inner>
  MLSMessage protect_full(Inner&& content, const MessageOpts& msg_opts);

  // Apply the changes requested by various messages
  LeafIndex apply(const Add& add);
  void apply(LeafIndex target, const Update& update);
  void apply(LeafIndex target,
             const Update& update,
             const HPKEPrivateKey& leaf_priv);
  LeafIndex apply(const Remove& remove);
  void apply(const GroupContextExtensions& gce);
  std::vector<LeafIndex> apply(const std::vector<CachedProposal>& proposals,
                               Proposal::Type required_type);
  std::tuple<std::vector<LeafIndex>, std::vector<PSKWithSecret>> apply(
    const std::vector<CachedProposal>& proposals);

  // Verify that a specific key package or all members support a given set of
  // extensions
  bool extensions_supported(const ExtensionList& exts) const;

  // Extract proposals and PSKs from cache
  void cache_proposal(AuthenticatedContent content_auth);
  std::optional<CachedProposal> resolve(
    const ProposalOrRef& id,
    std::optional<LeafIndex> sender_index) const;
  std::vector<CachedProposal> must_resolve(
    const std::vector<ProposalOrRef>& ids,
    std::optional<LeafIndex> sender_index) const;

  std::vector<PSKWithSecret> resolve(
    const std::vector<PreSharedKeyID>& psks) const;

  // Check properties of proposals
  bool valid(const LeafNode& leaf_node,
             LeafNodeSource required_source,
             std::optional<LeafIndex> index) const;
  bool valid(const KeyPackage& key_package) const;
  bool valid(const Add& add) const;
  bool valid(LeafIndex sender, const Update& update) const;
  bool valid(const Remove& remove) const;
  bool valid(const PreSharedKey& psk) const;
  static bool valid(const ReInit& reinit);
  bool valid(const ExternalInit& external_init) const;
  bool valid(const GroupContextExtensions& gce) const;
  bool valid(std::optional<LeafIndex> sender, const Proposal& proposal) const;

  bool valid(const std::vector<CachedProposal>& proposals,
             LeafIndex commit_sender,
             const CommitParams& params) const;
  bool valid_normal(const std::vector<CachedProposal>& proposals,
                    LeafIndex commit_sender) const;
  bool valid_external(const std::vector<CachedProposal>& proposals) const;
  static bool valid_reinit(const std::vector<CachedProposal>& proposals);
  static bool valid_restart(const std::vector<CachedProposal>& proposals,
                            ResumptionPSKUsage allowed_usage);

  static bool valid_external_proposal_type(const Proposal::Type proposal_type);

  CommitParams infer_commit_type(
    const std::optional<LeafIndex>& sender,
    const std::vector<CachedProposal>& proposals,
    const std::optional<CommitParams>& expected_params) const;
  static bool path_required(const std::vector<CachedProposal>& proposals);

  // Compare the **shared** attributes of the states
  friend bool operator==(const State& lhs, const State& rhs);
  friend bool operator!=(const State& lhs, const State& rhs);

  // Derive and set the secrets for an epoch, given some new entropy
  void update_epoch_secrets(const bytes& commit_secret,
                            const std::vector<PSKWithSecret>& psks,
                            const std::optional<bytes>& force_init_secret);

  // Signature verification over a handshake message
  bool verify_internal(const AuthenticatedContent& content_auth) const;
  bool verify_external(const AuthenticatedContent& content_auth) const;
  bool verify_new_member_proposal(
    const AuthenticatedContent& content_auth) const;
  bool verify_new_member_commit(const AuthenticatedContent& content_auth) const;
  bool verify(const AuthenticatedContent& content_auth) const;

  // Convert a Roster entry into LeafIndex
  LeafIndex leaf_for_roster_entry(RosterIndex index) const;

  // Create a draft successor state
  State successor() const;
};

} // namespace mlspp
