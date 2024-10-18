#pragma once

#include <bytes/bytes.h>
#include <mls/crypto.h>
#include <mls/key_schedule.h>
#include <mls/messages.h>
#include <mls/tree_math.h>
#include <mls/treekem.h>
#include <tls/tls_syntax.h>
#include <vector>

namespace mls_vectors {

struct PseudoRandom
{
  struct Generator
  {
    Generator() = default;
    Generator(mlspp::CipherSuite suite_in, const std::string& label);
    Generator sub(const std::string& label) const;

    bytes secret(const std::string& label) const;
    bytes generate(const std::string& label, size_t size) const;

    uint16_t uint16(const std::string& label) const;
    uint32_t uint32(const std::string& label) const;
    uint64_t uint64(const std::string& label) const;

    mlspp::SignaturePrivateKey signature_key(
      const std::string& label) const;
    mlspp::HPKEPrivateKey hpke_key(const std::string& label) const;

    size_t output_length() const;

  private:
    mlspp::CipherSuite suite;
    bytes seed;

    Generator(mlspp::CipherSuite suite_in, bytes seed_in);
  };

  PseudoRandom() = default;
  PseudoRandom(mlspp::CipherSuite suite, const std::string& label);

  Generator prg;
};

struct TreeMathTestVector
{
  using OptionalNode = std::optional<mlspp::NodeIndex>;

  mlspp::LeafCount n_leaves;
  mlspp::NodeCount n_nodes;
  mlspp::NodeIndex root;
  std::vector<OptionalNode> left;
  std::vector<OptionalNode> right;
  std::vector<OptionalNode> parent;
  std::vector<OptionalNode> sibling;

  std::optional<mlspp::NodeIndex> null_if_invalid(
    mlspp::NodeIndex input,
    mlspp::NodeIndex answer) const;

  TreeMathTestVector() = default;
  TreeMathTestVector(uint32_t n_leaves);
  std::optional<std::string> verify() const;
};

struct CryptoBasicsTestVector : PseudoRandom
{
  struct RefHash
  {
    std::string label;
    bytes value;
    bytes out;

    RefHash() = default;
    RefHash(mlspp::CipherSuite suite,
            const PseudoRandom::Generator& prg);
    std::optional<std::string> verify(mlspp::CipherSuite suite) const;
  };

  struct ExpandWithLabel
  {
    bytes secret;
    std::string label;
    bytes context;
    uint16_t length;
    bytes out;

    ExpandWithLabel() = default;
    ExpandWithLabel(mlspp::CipherSuite suite,
                    const PseudoRandom::Generator& prg);
    std::optional<std::string> verify(mlspp::CipherSuite suite) const;
  };

  struct DeriveSecret
  {
    bytes secret;
    std::string label;
    bytes out;

    DeriveSecret() = default;
    DeriveSecret(mlspp::CipherSuite suite,
                 const PseudoRandom::Generator& prg);
    std::optional<std::string> verify(mlspp::CipherSuite suite) const;
  };

  struct DeriveTreeSecret
  {
    bytes secret;
    std::string label;
    uint32_t generation;
    uint16_t length;
    bytes out;

    DeriveTreeSecret() = default;
    DeriveTreeSecret(mlspp::CipherSuite suite,
                     const PseudoRandom::Generator& prg);
    std::optional<std::string> verify(mlspp::CipherSuite suite) const;
  };

  struct SignWithLabel
  {
    mlspp::SignaturePrivateKey priv;
    mlspp::SignaturePublicKey pub;
    bytes content;
    std::string label;
    bytes signature;

    SignWithLabel() = default;
    SignWithLabel(mlspp::CipherSuite suite,
                  const PseudoRandom::Generator& prg);
    std::optional<std::string> verify(mlspp::CipherSuite suite) const;
  };

  struct EncryptWithLabel
  {
    mlspp::HPKEPrivateKey priv;
    mlspp::HPKEPublicKey pub;
    std::string label;
    bytes context;
    bytes plaintext;
    bytes kem_output;
    bytes ciphertext;

    EncryptWithLabel() = default;
    EncryptWithLabel(mlspp::CipherSuite suite,
                     const PseudoRandom::Generator& prg);
    std::optional<std::string> verify(mlspp::CipherSuite suite) const;
  };

  mlspp::CipherSuite cipher_suite;

  RefHash ref_hash;
  ExpandWithLabel expand_with_label;
  DeriveSecret derive_secret;
  DeriveTreeSecret derive_tree_secret;
  SignWithLabel sign_with_label;
  EncryptWithLabel encrypt_with_label;

  CryptoBasicsTestVector() = default;
  CryptoBasicsTestVector(mlspp::CipherSuite suite);
  std::optional<std::string> verify() const;
};

struct SecretTreeTestVector : PseudoRandom
{
  struct SenderData
  {
    bytes sender_data_secret;
    bytes ciphertext;
    bytes key;
    bytes nonce;

    SenderData() = default;
    SenderData(mlspp::CipherSuite suite,
               const PseudoRandom::Generator& prg);
    std::optional<std::string> verify(mlspp::CipherSuite suite) const;
  };

  struct RatchetStep
  {
    uint32_t generation;
    bytes handshake_key;
    bytes handshake_nonce;
    bytes application_key;
    bytes application_nonce;
  };

  mlspp::CipherSuite cipher_suite;

  SenderData sender_data;

  bytes encryption_secret;
  std::vector<std::vector<RatchetStep>> leaves;

  SecretTreeTestVector() = default;
  SecretTreeTestVector(mlspp::CipherSuite suite,
                       uint32_t n_leaves,
                       const std::vector<uint32_t>& generations);
  std::optional<std::string> verify() const;
};

struct KeyScheduleTestVector : PseudoRandom
{
  struct Export
  {
    std::string label;
    bytes context;
    size_t length;
    bytes secret;
  };

  struct Epoch
  {
    // Chosen by the generator
    bytes tree_hash;
    bytes commit_secret;
    bytes psk_secret;
    bytes confirmed_transcript_hash;

    // Computed values
    bytes group_context;

    bytes joiner_secret;
    bytes welcome_secret;
    bytes init_secret;

    bytes sender_data_secret;
    bytes encryption_secret;
    bytes exporter_secret;
    bytes epoch_authenticator;
    bytes external_secret;
    bytes confirmation_key;
    bytes membership_key;
    bytes resumption_psk;

    mlspp::HPKEPublicKey external_pub;
    Export exporter;
  };

  mlspp::CipherSuite cipher_suite;

  bytes group_id;
  bytes initial_init_secret;

  std::vector<Epoch> epochs;

  KeyScheduleTestVector() = default;
  KeyScheduleTestVector(mlspp::CipherSuite suite, uint32_t n_epochs);
  std::optional<std::string> verify() const;
};

struct MessageProtectionTestVector : PseudoRandom
{
  mlspp::CipherSuite cipher_suite;

  bytes group_id;
  mlspp::epoch_t epoch;
  bytes tree_hash;
  bytes confirmed_transcript_hash;

  mlspp::SignaturePrivateKey signature_priv;
  mlspp::SignaturePublicKey signature_pub;

  bytes encryption_secret;
  bytes sender_data_secret;
  bytes membership_key;

  mlspp::Proposal proposal;
  mlspp::MLSMessage proposal_pub;
  mlspp::MLSMessage proposal_priv;

  mlspp::Commit commit;
  mlspp::MLSMessage commit_pub;
  mlspp::MLSMessage commit_priv;

  bytes application;
  mlspp::MLSMessage application_priv;

  MessageProtectionTestVector() = default;
  MessageProtectionTestVector(mlspp::CipherSuite suite);
  std::optional<std::string> verify();

private:
  mlspp::GroupKeySource group_keys() const;
  mlspp::GroupContext group_context() const;

  mlspp::MLSMessage protect_pub(
    const mlspp::GroupContent::RawContent& raw_content) const;
  mlspp::MLSMessage protect_priv(
    const mlspp::GroupContent::RawContent& raw_content);
  std::optional<mlspp::GroupContent> unprotect(
    const mlspp::MLSMessage& message);
};

struct PSKSecretTestVector : PseudoRandom
{
  struct PSK
  {
    bytes psk_id;
    bytes psk_nonce;
    bytes psk;
  };

  mlspp::CipherSuite cipher_suite;
  std::vector<PSK> psks;
  bytes psk_secret;

  PSKSecretTestVector() = default;
  PSKSecretTestVector(mlspp::CipherSuite suite, size_t n_psks);
  std::optional<std::string> verify() const;
};

struct TranscriptTestVector : PseudoRandom
{
  mlspp::CipherSuite cipher_suite;

  bytes confirmation_key;
  bytes interim_transcript_hash_before;

  mlspp::AuthenticatedContent authenticated_content;

  bytes confirmed_transcript_hash_after;
  bytes interim_transcript_hash_after;

  TranscriptTestVector() = default;
  TranscriptTestVector(mlspp::CipherSuite suite);
  std::optional<std::string> verify() const;
};

struct WelcomeTestVector : PseudoRandom
{
  mlspp::CipherSuite cipher_suite;

  mlspp::HPKEPrivateKey init_priv;
  mlspp::SignaturePublicKey signer_pub;

  mlspp::MLSMessage key_package;
  mlspp::MLSMessage welcome;

  WelcomeTestVector() = default;
  WelcomeTestVector(mlspp::CipherSuite suite);
  std::optional<std::string> verify() const;
};

// XXX(RLB): The |structure| of the example trees below is to avoid compile
// errors from gcc's -Werror=comment when there is a '\' character at the end of
// a line.  Inspired by a similar bug in Chromium:
//   https://codereview.chromium.org/874663003/patch/1/10001
enum struct TreeStructure
{
  // Full trees on N leaves, created by member k adding member k+1
  full_tree_2,
  full_tree_3,
  full_tree_4,
  full_tree_5,
  full_tree_6,
  full_tree_7,
  full_tree_8,
  full_tree_32,
  full_tree_33,
  full_tree_34,

  // |               W               |
  // |         ______|______         |
  // |        /             \        |
  // |       U               Y       |
  // |     __|__           __|__     |
  // |    /     \         /     \    |
  // |   T       _       X       Z   |
  // |  / \     / \     / \     / \  |
  // | A   B   C   _   E   F   G   H |
  //
  // * Start with full tree on 8 members
  // * 0 commits removeing 2 and 3, and adding a new member
  internal_blanks_no_skipping,

  // |               W               |
  // |         ______|______         |
  // |        /             \        |
  // |       _               Y       |
  // |     __|__           __|__     |
  // |    /     \         /     \    |
  // |   _       _       X       Z   |
  // |  / \     / \     / \     / \  |
  // | A   _   _   _   E   F   G   H |
  //
  // * Start with full tree on 8 members
  // * 0 commitsremoveing 1, 2, and 3
  internal_blanks_with_skipping,

  // |               W[H]            |
  // |         ______|______         |
  // |        /             \        |
  // |       U               Y[H]    |
  // |     __|__           __|__     |
  // |    /     \         /     \    |
  // |   T       V       X       _   |
  // |  / \     / \     / \     / \  |
  // | A   B   C   D   E   F   G   H |
  //
  // * Start with full tree on 7 members
  // * 0 commits adding a member in a partial Commit (no path)
  unmerged_leaves_no_skipping,

  // |               W [F]           |
  // |         ______|______         |
  // |        /             \        |
  // |       U               Y [F]   |
  // |     __|__           __|__     |
  // |    /     \         /     \    |
  // |   T       _       _       _   |
  // |  / \     / \     / \     / \  |
  // | A   B   C   D   E   F   G   _ |
  //
  // == Fig. 20 / {{parent-hash-tree}}
  // * 0 creates group
  // * 0 adds 1, ..., 6 in a partial Commit
  // * O commits removing 5
  // * 4 commits without any proposals
  // * 0 commits adding a new member in a partial Commit
  unmerged_leaves_with_skipping,
};

extern std::array<TreeStructure, 14> all_tree_structures;
extern std::array<TreeStructure, 11> treekem_test_tree_structures;

struct TreeHashTestVector : PseudoRandom
{
  mlspp::CipherSuite cipher_suite;
  bytes group_id;

  mlspp::TreeKEMPublicKey tree;
  std::vector<bytes> tree_hashes;
  std::vector<std::vector<mlspp::NodeIndex>> resolutions;

  TreeHashTestVector() = default;
  TreeHashTestVector(mlspp::CipherSuite suite,
                     TreeStructure tree_structure);
  std::optional<std::string> verify();
};

struct TreeOperationsTestVector : PseudoRandom
{
  enum struct Scenario
  {
    add_right_edge,
    add_internal,
    update,
    remove_right_edge,
    remove_internal,
  };

  static const std::vector<Scenario> all_scenarios;

  mlspp::CipherSuite cipher_suite;

  mlspp::TreeKEMPublicKey tree_before;
  bytes tree_hash_before;

  mlspp::Proposal proposal;
  mlspp::LeafIndex proposal_sender;

  mlspp::TreeKEMPublicKey tree_after;
  bytes tree_hash_after;

  TreeOperationsTestVector() = default;
  TreeOperationsTestVector(mlspp::CipherSuite suite, Scenario scenario);
  std::optional<std::string> verify();
};

struct TreeKEMTestVector : PseudoRandom
{
  struct PathSecret
  {
    mlspp::NodeIndex node;
    bytes path_secret;
  };

  struct LeafPrivateInfo
  {
    mlspp::LeafIndex index;
    mlspp::HPKEPrivateKey encryption_priv;
    mlspp::SignaturePrivateKey signature_priv;
    std::vector<PathSecret> path_secrets;
  };

  struct UpdatePathInfo
  {
    mlspp::LeafIndex sender;
    mlspp::UpdatePath update_path;
    std::vector<std::optional<bytes>> path_secrets;
    bytes commit_secret;
    bytes tree_hash_after;
  };

  mlspp::CipherSuite cipher_suite;

  bytes group_id;
  mlspp::epoch_t epoch;
  bytes confirmed_transcript_hash;

  mlspp::TreeKEMPublicKey ratchet_tree;

  std::vector<LeafPrivateInfo> leaves_private;
  std::vector<UpdatePathInfo> update_paths;

  TreeKEMTestVector() = default;
  TreeKEMTestVector(mlspp::CipherSuite suite,
                    TreeStructure tree_structure);
  std::optional<std::string> verify();
};

struct MessagesTestVector : PseudoRandom
{
  bytes mls_welcome;
  bytes mls_group_info;
  bytes mls_key_package;

  bytes ratchet_tree;
  bytes group_secrets;

  bytes add_proposal;
  bytes update_proposal;
  bytes remove_proposal;
  bytes pre_shared_key_proposal;
  bytes re_init_proposal;
  bytes external_init_proposal;
  bytes group_context_extensions_proposal;

  bytes commit;

  bytes public_message_proposal;
  bytes public_message_commit;
  bytes private_message;

  MessagesTestVector();
  std::optional<std::string> verify() const;
};

struct PassiveClientTestVector : PseudoRandom
{
  struct PSK
  {
    bytes psk_id;
    bytes psk;
  };

  struct Epoch
  {
    std::vector<mlspp::MLSMessage> proposals;
    mlspp::MLSMessage commit;
    bytes epoch_authenticator;
  };

  mlspp::CipherSuite cipher_suite;

  mlspp::MLSMessage key_package;
  mlspp::SignaturePrivateKey signature_priv;
  mlspp::HPKEPrivateKey encryption_priv;
  mlspp::HPKEPrivateKey init_priv;

  std::vector<PSK> external_psks;

  mlspp::MLSMessage welcome;
  std::optional<mlspp::TreeKEMPublicKey> ratchet_tree;
  bytes initial_epoch_authenticator;

  std::vector<Epoch> epochs;

  PassiveClientTestVector() = default;
  std::optional<std::string> verify();
};

} // namespace mls_vectors
