#include <mls/treekem.h>

#if ENABLE_TREE_DUMP
#include <iostream>
#endif

namespace mlspp {

// Utility method used for removing leaves from a resolution
static void
remove_leaves(std::vector<NodeIndex>& res, const std::vector<LeafIndex>& except)
{
  for (const auto& leaf : except) {
    auto it = std::find(res.begin(), res.end(), NodeIndex(leaf));
    if (it == res.end()) {
      continue;
    }

    res.erase(it);
  }
}

///
/// Node
///

const HPKEPublicKey&
Node::public_key() const
{
  const auto get_key = overloaded{
    [](const LeafNode& n) -> const HPKEPublicKey& { return n.encryption_key; },
    [](const ParentNode& n) -> const HPKEPublicKey& { return n.public_key; },
  };
  return var::visit(get_key, node);
}

std::optional<bytes>
Node::parent_hash() const
{
  const auto get_leaf_ph = overloaded{
    [](const ParentHash& ph) -> std::optional<bytes> { return ph.parent_hash; },
    [](const auto& /* other */) -> std::optional<bytes> {
      return std::nullopt;
    },
  };

  const auto get_ph = overloaded{
    [&](const LeafNode& node) -> std::optional<bytes> {
      return var::visit(get_leaf_ph, node.content);
    },
    [](const ParentNode& node) -> std::optional<bytes> {
      return node.parent_hash;
    },
  };

  return var::visit(get_ph, node);
}

///
/// TreeKEMPrivateKey
///

TreeKEMPrivateKey
TreeKEMPrivateKey::solo(CipherSuite suite,
                        LeafIndex index,
                        HPKEPrivateKey leaf_priv)
{
  auto priv = TreeKEMPrivateKey{ suite, index, {}, {}, {} };
  priv.private_key_cache.insert({ NodeIndex(index), std::move(leaf_priv) });
  return priv;
}

TreeKEMPrivateKey
TreeKEMPrivateKey::create(const TreeKEMPublicKey& pub,
                          LeafIndex from,
                          const bytes& leaf_secret)
{
  auto priv = TreeKEMPrivateKey{ pub.suite, from, {}, {}, {} };
  priv.implant(pub, NodeIndex(from), leaf_secret);
  return priv;
}

TreeKEMPrivateKey
TreeKEMPrivateKey::joiner(const TreeKEMPublicKey& pub,
                          LeafIndex index,
                          HPKEPrivateKey leaf_priv,
                          NodeIndex intersect,
                          const std::optional<bytes>& path_secret)
{
  auto priv = TreeKEMPrivateKey{ pub.suite, index, {}, {}, {} };
  priv.private_key_cache.insert({ NodeIndex(index), std::move(leaf_priv) });
  if (path_secret) {
    priv.implant(pub, intersect, opt::get(path_secret));
  }
  return priv;
}

void
TreeKEMPrivateKey::implant(const TreeKEMPublicKey& pub,
                           NodeIndex start,
                           const bytes& path_secret)
{
  const auto fdp = pub.filtered_direct_path(start);
  auto secret = path_secret;

  path_secrets.insert_or_assign(start, secret);
  private_key_cache.erase(start);

  for (const auto& [n, _res] : fdp) {
    secret = pub.suite.derive_secret(secret, "path");
    path_secrets.insert_or_assign(n, secret);
    private_key_cache.erase(n);
  }

  update_secret = pub.suite.derive_secret(secret, "path");
}

std::optional<HPKEPrivateKey>
TreeKEMPrivateKey::private_key(NodeIndex n) const
{
  auto pki = private_key_cache.find(n);
  if (pki != private_key_cache.end()) {
    return pki->second;
  }

  auto i = path_secrets.find(n);
  if (i == path_secrets.end()) {
    return std::nullopt;
  }

  auto node_secret = suite.derive_secret(i->second, "node");
  return HPKEPrivateKey::derive(suite, node_secret);
}

bool
TreeKEMPrivateKey::have_private_key(NodeIndex n) const
{
  auto path_secret = path_secrets.find(n) != path_secrets.end();
  auto cached_priv = private_key_cache.find(n) != private_key_cache.end();
  return path_secret || cached_priv;
}

std::optional<HPKEPrivateKey>
TreeKEMPrivateKey::private_key(NodeIndex n)
{
  auto priv = static_cast<const TreeKEMPrivateKey&>(*this).private_key(n);
  if (priv) {
    private_key_cache.insert_or_assign(n, opt::get(priv));
  }
  return priv;
}

void
TreeKEMPrivateKey::set_leaf_priv(HPKEPrivateKey priv)
{
  auto n = NodeIndex(index);
  path_secrets.erase(n);
  private_key_cache.insert_or_assign(n, std::move(priv));
}

std::tuple<NodeIndex, bytes, bool>
TreeKEMPrivateKey::shared_path_secret(LeafIndex to) const
{
  auto n = index.ancestor(to);
  auto i = path_secrets.find(n);
  if (i == path_secrets.end()) {
    return std::make_tuple(n, bytes{}, false);
  }

  return std::make_tuple(n, i->second, true);
}

#if ENABLE_TREE_DUMP
// XXX(RLB) This should ultimately be deleted, but it is handy for interop
// debugging, so I'm keeping it around for now.  If re-enabled, you'll also need
// to add the appropriate declarations to treekem.h and include <iostream>

void
TreeKEMPrivateKey::dump() const
{
  for (const auto& [node, _] : path_secrets) {
    private_key(node);
  }

  std::cout << "Tree (priv):" << std::endl;
  std::cout << "  Index: " << NodeIndex(index).val << std::endl;

  std::cout << "  Secrets: " << std::endl;
  for (const auto& [n, path_secret] : path_secrets) {
    auto node_secret = suite.derive_secret(path_secret, "node");
    auto sk = HPKEPrivateKey::derive(suite, node_secret);

    auto psm = to_hex(path_secret).substr(0, 8);
    auto pkm = to_hex(sk.public_key.data).substr(0, 8);
    std::cout << "    " << n.val << " => " << psm << " => " << pkm << std::endl;
  }

  std::cout << "  Cached key pairs: " << std::endl;
  for (const auto& [n, sk] : private_key_cache) {
    auto pkm = to_hex(sk.public_key.data).substr(0, 8);
    std::cout << "    " << n.val << " => " << pkm << std::endl;
  }
}

void
TreeKEMPublicKey::dump() const
{
  std::cout << "Tree:" << std::endl;
  auto width = NodeCount(size);
  for (auto i = NodeIndex{ 0 }; i.val < width.val; i.val++) {
    printf("  %03d : ", i.val); // NOLINT
    if (!node_at(i).blank()) {
      auto pkRm = to_hex(opt::get(node_at(i).node).public_key().data);
      std::cout << pkRm.substr(0, 8);
    } else {
      std::cout << "        ";
    }

    std::cout << "  | ";
    for (uint32_t j = 0; j < i.level(); j++) {
      std::cout << "  ";
    }

    if (!node_at(i).blank()) {
      std::cout << "X";

      if (!i.is_leaf()) {
        auto parent = node_at(i).parent_node();
        std::cout << " [";
        for (const auto u : parent.unmerged_leaves) {
          std::cout << u.val << ", ";
        }
        std::cout << "]";
      }

    } else {
      std::cout << "_";
    }

    std::cout << std::endl;
  }
}
#endif

void
TreeKEMPrivateKey::decap(LeafIndex from,
                         const TreeKEMPublicKey& pub,
                         const bytes& context,
                         const UpdatePath& path,
                         const std::vector<LeafIndex>& except)
{
  // Identify which node in the path secret we will be decrypting
  auto ni = NodeIndex(index);
  auto dp = pub.filtered_direct_path(NodeIndex(from));
  if (dp.size() != path.nodes.size()) {
    throw ProtocolError("Malformed direct path");
  }

  size_t dpi = 0;
  auto overlap_node = NodeIndex{};
  auto res = std::vector<NodeIndex>{};
  for (dpi = 0; dpi < dp.size(); dpi++) {
    const auto [dpn, dpres] = dp[dpi];
    if (ni.is_below(dpn)) {
      overlap_node = dpn;
      res = dpres;
      break;
    }
  }

  if (dpi == dp.size()) {
    throw ProtocolError("No overlap in path");
  }

  // Identify which node in the resolution of the copath we will use to decrypt
  remove_leaves(res, except);
  if (res.size() != path.nodes[dpi].encrypted_path_secret.size()) {
    throw ProtocolError("Malformed direct path node");
  }

  size_t resi = 0;
  const NodeIndex res_overlap_node;
  for (resi = 0; resi < res.size(); resi++) {
    if (have_private_key(res[resi])) {
      break;
    }
  }

  if (resi == res.size()) {
    throw ProtocolError("No private key to decrypt path secret");
  }

  // Decrypt and implant
  auto priv = opt::get(private_key(res[resi]));
  auto path_secret = priv.decrypt(suite,
                                  encrypt_label::update_path_node,
                                  context,
                                  path.nodes[dpi].encrypted_path_secret[resi]);
  implant(pub, overlap_node, path_secret);

  // Check that the resulting state is consistent with the public key
  if (!consistent(pub)) {
    throw ProtocolError("TreeKEMPublicKey inconsistent with TreeKEMPrivateKey");
  }
}

void
TreeKEMPrivateKey::truncate(LeafCount size)
{
  auto ni = NodeIndex(LeafIndex{ size.val - 1 });
  auto to_remove = std::vector<NodeIndex>{};
  for (const auto& entry : path_secrets) {
    if (entry.first.val > ni.val) {
      to_remove.push_back(entry.first);
    }
  }

  for (auto n : to_remove) {
    path_secrets.erase(n);
    private_key_cache.erase(n);
  }
}

bool
TreeKEMPrivateKey::consistent(const TreeKEMPrivateKey& other) const
{
  if (suite != other.suite) {
    return false;
  }

  if (update_secret != other.update_secret) {
    return false;
  }

  const auto match_if_present = [&](const auto& entry) {
    auto other_entry = other.path_secrets.find(entry.first);
    if (other_entry == other.path_secrets.end()) {
      return true;
    }

    return entry.second == other_entry->second;
  };
  return stdx::all_of(path_secrets, match_if_present);
}

bool
TreeKEMPrivateKey::consistent(const TreeKEMPublicKey& other) const
{
  if (suite != other.suite) {
    return false;
  }

  for (const auto& [node, _] : path_secrets) {
    private_key(node);
  }

  return stdx::all_of(private_key_cache, [other](const auto& entry) {
    const auto& [node, priv] = entry;
    const auto& opt_node = other.node_at(node).node;
    if (!opt_node) {
      // It's OK for a TreeKEMPrivateKey to have private keys
      // for nodes that are blank in the TreeKEMPublicKey.
      // This will happen traniently during Commit
      // processing, since proposals will be applied in the
      // public tree and not in the private tree.
      return true;
    }

    const auto& pub = opt::get(opt_node).public_key();
    return priv.public_key == pub;
  });
}

///
/// TreeKEMPublicKey
///

TreeKEMPublicKey::TreeKEMPublicKey(CipherSuite suite_in)
  : suite(suite_in)
{
}

LeafIndex
TreeKEMPublicKey::allocate_leaf()
{
  // Find the leftmost blank leaf node
  auto index = LeafIndex(0);
  while (index.val < size.val && !node_at(index).blank()) {
    index.val++;
  }

  // Extend the tree if necessary
  if (index.val >= size.val) {
    if (size.val == 0) {
      size.val = 1;
      nodes.resize(1);
    } else {
      size.val *= 2;
      nodes.resize(2 * nodes.size() + 1);
    }
  }

  return index;
}

LeafIndex
TreeKEMPublicKey::add_leaf(const LeafNode& leaf)
{
  // Check that the leaf node's keys are not already present in the tree
  if (exists_in_tree(leaf.encryption_key, std::nullopt)) {
    throw InvalidParameterError("Duplicate encryption key");
  }

  if (exists_in_tree(leaf.signature_key, std::nullopt)) {
    throw InvalidParameterError("Duplicate signature key");
  }

  // Allocate a blank leaf for this node
  const auto index = allocate_leaf();

  // Set the leaf
  node_at(index).node = Node{ leaf };

  // Update the unmerged list
  for (auto& n : NodeIndex(index).dirpath(size)) {
    if (!node_at(n).node) {
      continue;
    }

    auto& parent = var::get<ParentNode>(opt::get(node_at(n).node).node);

    // Insert into unmerged leaves while maintaining order
    const auto insert_point = stdx::upper_bound(parent.unmerged_leaves, index);
    parent.unmerged_leaves.insert(insert_point, index);
  }

  clear_hash_path(index);
  return index;
}

void
TreeKEMPublicKey::update_leaf(LeafIndex index, const LeafNode& leaf)
{
  // Check that the leaf node's keys are not already present in the tree, except
  // for the signature key, which is allowed to repeat.
  if (exists_in_tree(leaf.encryption_key, std::nullopt)) {
    throw InvalidParameterError("Duplicate encryption key");
  }

  if (exists_in_tree(leaf.signature_key, index)) {
    throw InvalidParameterError("Duplicate signature key");
  }

  blank_path(index);
  node_at(NodeIndex(index)).node = Node{ leaf };
  clear_hash_path(index);
}

void
TreeKEMPublicKey::blank_path(LeafIndex index)
{
  if (nodes.empty()) {
    return;
  }

  auto ni = NodeIndex(index);
  node_at(ni).node.reset();
  for (auto n : ni.dirpath(size)) {
    node_at(n).node.reset();
  }

  clear_hash_path(index);
}

void
TreeKEMPublicKey::merge(LeafIndex from, const UpdatePath& path)
{
  update_leaf(from, path.leaf_node);

  auto dp = filtered_direct_path(NodeIndex(from));
  if (dp.size() != path.nodes.size()) {
    throw ProtocolError("Malformed direct path");
  }

  auto ph = parent_hashes(from, dp, path.nodes);
  for (size_t i = 0; i < dp.size(); i++) {
    auto [n, _res] = dp[i];

    auto parent_hash = bytes{};
    if (i < dp.size() - 1) {
      parent_hash = ph[i + 1];
    }

    node_at(n).node =
      Node{ ParentNode{ path.nodes[i].public_key, parent_hash, {} } };
  }

  set_hash_all();
}

void
TreeKEMPublicKey::set_hash_all()
{
  auto r = NodeIndex::root(size);
  get_hash(r);
}

bytes
TreeKEMPublicKey::root_hash() const
{
  auto r = NodeIndex::root(size);
  if (hashes.count(r) == 0) {
    throw InvalidParameterError("Root hash not set");
  }

  return hashes.at(r);
}

bool
TreeKEMPublicKey::has_parent_hash(NodeIndex child, const bytes& target_ph) const
{
  const auto res = resolve(child);
  return stdx::any_of(res, [&](auto nr) {
    return opt::get(node_at(nr).node).parent_hash() == target_ph;
  });
}

bool
TreeKEMPublicKey::parent_hash_valid() const
{
  auto cache = TreeHashCache{};

  auto width = NodeCount(size);
  auto height = NodeIndex::root(size).level();
  for (auto level = uint32_t(1); level <= height; level++) {
    auto stride = uint32_t(2) << level;
    auto start = NodeIndex{ (stride >> 1U) - 1 };

    for (auto p = start; p.val < width.val; p.val += stride) {
      if (node_at(p).blank()) {
        continue;
      }

      auto l = p.left();
      auto r = p.right();

      auto lh = original_parent_hash(cache, p, r);
      auto rh = original_parent_hash(cache, p, l);

      if (!has_parent_hash(l, lh) && !has_parent_hash(r, rh)) {
        dump();
        return false;
      }
    }
  }
  return true;
}

std::vector<NodeIndex>
TreeKEMPublicKey::resolve(NodeIndex index) const
{
  auto at_leaf = (index.level() == 0);
  if (!node_at(index).blank()) {
    auto out = std::vector<NodeIndex>{ index };
    if (index.is_leaf()) {
      return out;
    }

    const auto& node = node_at(index);
    auto unmerged =
      stdx::transform<NodeIndex>(node.parent_node().unmerged_leaves,
                                 [](LeafIndex x) { return NodeIndex(x); });

    out.insert(out.end(), unmerged.begin(), unmerged.end());
    return out;
  }

  if (at_leaf) {
    return {};
  }

  auto l = resolve(index.left());
  auto r = resolve(index.right());
  l.insert(l.end(), r.begin(), r.end());
  return l;
}

TreeKEMPublicKey::FilteredDirectPath
TreeKEMPublicKey::filtered_direct_path(NodeIndex index) const
{
  auto fdp = FilteredDirectPath{};

  const auto cp = index.copath(size);
  auto last = index;
  for (auto n : cp) {
    const auto p = n.parent();
    const auto res = resolve(n);
    last = p;
    if (res.empty()) {
      continue;
    }

    fdp.emplace_back(p, res);
  }

  return fdp;
}

bool
TreeKEMPublicKey::has_leaf(LeafIndex index) const
{
  return !node_at(index).blank();
}

std::optional<LeafIndex>
TreeKEMPublicKey::find(const LeafNode& leaf) const
{
  for (LeafIndex i{ 0 }; i < size; i.val++) {
    const auto& node = node_at(i);
    if (!node.blank() && node.leaf_node() == leaf) {
      return i;
    }
  }

  return std::nullopt;
}

std::optional<LeafNode>
TreeKEMPublicKey::leaf_node(LeafIndex index) const
{
  const auto& node = node_at(index);
  if (node.blank()) {
    return std::nullopt;
  }

  return node.leaf_node();
}

TreeKEMPrivateKey
TreeKEMPublicKey::update(LeafIndex from,
                         const bytes& leaf_secret,
                         const bytes& group_id,
                         const SignaturePrivateKey& sig_priv,
                         const LeafNodeOptions& opts)
{
  // Grab information about the sender
  const auto& leaf_node = node_at(from);
  if (leaf_node.blank()) {
    throw InvalidParameterError("Cannot update from blank node");
  }

  // Generate path secrets
  auto priv = TreeKEMPrivateKey::create(*this, from, leaf_secret);
  auto dp = filtered_direct_path(NodeIndex(from));

  // Encrypt path secrets to the copath, forming a stub UpdatePath with no
  // encryptions
  auto path_nodes = stdx::transform<UpdatePathNode>(dp, [&](const auto& dpn) {
    auto [n, _res] = dpn;

    auto path_secret = priv.path_secrets.at(n);
    auto node_priv = opt::get(priv.private_key(n));

    return UpdatePathNode{ node_priv.public_key, {} };
  });

  // Update and re-sign the leaf_node
  auto ph = parent_hashes(from, dp, path_nodes);
  auto ph0 = bytes{};
  if (!ph.empty()) {
    ph0 = ph[0];
  }

  auto leaf_pub = opt::get(priv.private_key(NodeIndex(from))).public_key;
  auto new_leaf = leaf_node.leaf_node().for_commit(
    suite, group_id, from, leaf_pub, ph0, opts, sig_priv);

  // Merge the changes into the tree
  merge(from, UpdatePath{ std::move(new_leaf), std::move(path_nodes) });

  return priv;
}

UpdatePath
TreeKEMPublicKey::encap(const TreeKEMPrivateKey& priv,
                        const bytes& context,
                        const std::vector<LeafIndex>& except) const
{
  auto dp = filtered_direct_path(NodeIndex(priv.index));

  // Encrypt path secrets to the copath
  auto path_nodes = stdx::transform<UpdatePathNode>(dp, [&](const auto& dpn) {
    // We need the copy here so that we can modify the resolution.
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    auto [n, res] = dpn;
    remove_leaves(res, except);

    auto path_secret = priv.path_secrets.at(n);
    auto node_priv = opt::get(priv.private_key(n));

    auto ct = stdx::transform<HPKECiphertext>(res, [&](auto nr) {
      const auto& node_pub = opt::get(node_at(nr).node).public_key();
      return node_pub.encrypt(
        suite, encrypt_label::update_path_node, context, path_secret);
    });

    return UpdatePathNode{ node_priv.public_key, std::move(ct) };
  });

  // Package everything into an UpdatePath
  auto new_leaf = opt::get(leaf_node(priv.index));
  auto path = UpdatePath{ new_leaf, std::move(path_nodes) };

  return path;
}

void
TreeKEMPublicKey::truncate()
{
  if (size.val == 0) {
    return;
  }

  // Clear the parent hashes across blank leaves before truncating
  auto index = LeafIndex{ size.val - 1 };
  for (; index.val > 0; index.val--) {
    if (!node_at(index).blank()) {
      break;
    }
    clear_hash_path(index);
  }

  if (node_at(index).blank()) {
    nodes.clear();
    return;
  }

  // Remove the right subtree until the tree is of minimal size
  while (size.val / 2 > index.val) {
    nodes.resize(nodes.size() / 2);
    size.val /= 2;
  }
}

OptionalNode&
TreeKEMPublicKey::node_at(NodeIndex n)
{
  auto width = NodeCount(size);
  if (n.val >= width.val) {
    throw InvalidParameterError("Node index not in tree");
  }

  if (n.val >= nodes.size()) {
    return blank_node;
  }

  return nodes.at(n.val);
}

const OptionalNode&
TreeKEMPublicKey::node_at(NodeIndex n) const
{
  auto width = NodeCount(size);
  if (n.val >= width.val) {
    throw InvalidParameterError("Node index not in tree");
  }

  if (n.val >= nodes.size()) {
    return blank_node;
  }

  return nodes.at(n.val);
}

OptionalNode&
TreeKEMPublicKey::node_at(LeafIndex n)
{
  return node_at(NodeIndex(n));
}

const OptionalNode&
TreeKEMPublicKey::node_at(LeafIndex n) const
{
  return node_at(NodeIndex(n));
}

void
TreeKEMPublicKey::clear_hash_all()
{
  hashes.clear();
}

void
TreeKEMPublicKey::clear_hash_path(LeafIndex index)
{
  auto dp = NodeIndex(index).dirpath(size);
  hashes.erase(NodeIndex(index));
  for (auto n : dp) {
    hashes.erase(n);
  }
}

struct LeafNodeHashInput
{
  LeafIndex leaf_index;
  std::optional<LeafNode> leaf_node;
  TLS_SERIALIZABLE(leaf_index, leaf_node)
};

struct ParentNodeHashInput
{
  std::optional<ParentNode> parent_node;
  const bytes& left_hash;
  const bytes& right_hash;
  TLS_SERIALIZABLE(parent_node, left_hash, right_hash)
};

struct TreeHashInput
{
  var::variant<LeafNodeHashInput, ParentNodeHashInput> node;
  TLS_SERIALIZABLE(node);
  TLS_TRAITS(tls::variant<NodeType>)
};

const bytes&
TreeKEMPublicKey::get_hash(NodeIndex index)
{
  if (hashes.count(index) > 0) {
    return hashes.at(index);
  }

  auto hash_input = bytes{};
  const auto& node = node_at(index);
  if (index.level() == 0) {
    auto input = LeafNodeHashInput{ LeafIndex(index), {} };
    if (!node.blank()) {
      input.leaf_node = node.leaf_node();
    }

    hash_input = tls::marshal(TreeHashInput{ input });
  } else {
    auto input = ParentNodeHashInput{
      {},
      get_hash(index.left()),
      get_hash(index.right()),
    };

    if (!node.blank()) {
      input.parent_node = node.parent_node();
    }

    hash_input = tls::marshal(TreeHashInput{ input });
  }

  auto hash = suite.digest().hash(hash_input);
  hashes.insert_or_assign(index, hash);
  return hashes.at(index);
}

// struct {
//     HPKEPublicKey encryption_key;
//     opaque parent_hash<V>;
//     opaque original_sibling_tree_hash<V>;
// } ParentHashInput;
struct ParentHashInput
{
  const HPKEPublicKey& public_key;
  const bytes& parent_hash;
  const bytes& original_child_resolution;

  TLS_SERIALIZABLE(public_key, parent_hash, original_child_resolution)
};

bytes
TreeKEMPublicKey::parent_hash(const ParentNode& parent,
                              NodeIndex copath_child) const
{
  if (hashes.count(copath_child) == 0) {
    throw InvalidParameterError("Child hash not set");
  }

  auto hash_input = ParentHashInput{
    parent.public_key,
    parent.parent_hash,
    hashes.at(copath_child),
  };

  return suite.digest().hash(tls::marshal(hash_input));
}

std::vector<bytes>
TreeKEMPublicKey::parent_hashes(
  LeafIndex from,
  const FilteredDirectPath& fdp,
  const std::vector<UpdatePathNode>& path_nodes) const
{
  // An empty filtered direct path indicates a one-member tree, since there's
  // nobody else there to encrypt with.  In this special case, there's no
  // parent hashing to be done.
  if (fdp.empty()) {
    return {};
  }

  // The list of nodes for whom parent hashes are computed, namely: Direct path
  // excluding the last entry, including leaf
  auto from_node = NodeIndex(from);
  auto dp = fdp;
  auto [last, _res_last] = dp.back();
  dp.pop_back();
  dp.insert(dp.begin(), { from_node, {} });

  if (dp.size() != path_nodes.size()) {
    throw ProtocolError("Malformed UpdatePath");
  }

  // Parent hash for all the parents, starting from the last entry of the
  // filtered direct path
  auto last_hash = bytes{};
  auto ph = std::vector<bytes>(dp.size());
  for (int i = static_cast<int>(dp.size()) - 1; i >= 0; i--) {
    auto [n, _res] = dp[i];
    auto s = n.sibling(last);

    auto parent_node = ParentNode{ path_nodes[i].public_key, last_hash, {} };
    last_hash = parent_hash(parent_node, s);
    ph[i] = last_hash;

    last = n;
  }

  return ph;
}

const bytes&
TreeKEMPublicKey::original_tree_hash(TreeHashCache& cache,
                                     NodeIndex index,
                                     std::vector<LeafIndex> parent_except) const
{
  // Scope the unmerged leaves list down to this subtree
  auto except = std::vector<LeafIndex>{};
  std::copy_if(parent_except.begin(),
               parent_except.end(),
               std::back_inserter(except),
               [&](auto i) { return NodeIndex(i).is_below(index); });

  auto have_local_changes = !except.empty();

  // If there are no local changes, then we can use the cached tree hash
  if (!have_local_changes) {
    return hashes.at(index);
  }

  // If this method has been called before with the same number of excluded
  // leaves (which implies the same set), then use the cached value.
  if (auto it = cache.find(index); it != cache.end()) {
    const auto& [key, value] = *it;
    const auto& [except_size, hash] = value;
    if (except_size == except.size()) {
      return hash;
    }
  }

  // If there is no entry in either cache, recompute the value
  auto hash = bytes{};
  if (index.is_leaf()) {
    // A leaf node with local changes is by definition excluded from the parent
    // hash.  So we return the hash of an empty leaf.
    auto leaf_hash_input = LeafNodeHashInput{ LeafIndex(index), std::nullopt };
    hash = suite.digest().hash(tls::marshal(TreeHashInput{ leaf_hash_input }));
  } else {
    // If there is no cached value, recalculate the child hashes with the
    // specified `except` list, removing the `except` list from
    // `unmerged_leaves`.
    auto parent_hash_input = ParentNodeHashInput{
      std::nullopt,
      original_tree_hash(cache, index.left(), except),
      original_tree_hash(cache, index.right(), except),
    };

    if (!node_at(index).blank()) {
      parent_hash_input.parent_node = node_at(index).parent_node();
      auto& unmerged_leaves =
        opt::get(parent_hash_input.parent_node).unmerged_leaves;
      auto end = std::remove_if(
        unmerged_leaves.begin(), unmerged_leaves.end(), [&](auto leaf) {
          return std::count(except.begin(), except.end(), leaf) != 0;
        });
      unmerged_leaves.erase(end, unmerged_leaves.end());
    }

    hash =
      suite.digest().hash(tls::marshal(TreeHashInput{ parent_hash_input }));
  }

  cache.insert_or_assign(index, std::make_pair(except.size(), hash));
  return cache.at(index).second;
}

bytes
TreeKEMPublicKey::original_parent_hash(TreeHashCache& cache,
                                       NodeIndex parent,
                                       NodeIndex sibling) const
{
  const auto& parent_node = node_at(parent).parent_node();
  const auto& unmerged = parent_node.unmerged_leaves;
  const auto& sibling_hash = original_tree_hash(cache, sibling, unmerged);

  return suite.digest().hash(tls::marshal(ParentHashInput{
    parent_node.public_key,
    parent_node.parent_hash,
    sibling_hash,
  }));
}

bool
TreeKEMPublicKey::parent_hash_valid(LeafIndex from,
                                    const UpdatePath& path) const
{
  auto fdp = filtered_direct_path(NodeIndex(from));
  auto hash_chain = parent_hashes(from, fdp, path.nodes);
  auto leaf_ph =
    var::visit(overloaded{
                 [](const ParentHash& ph) -> std::optional<bytes> {
                   return ph.parent_hash;
                 },
                 [](const auto& /* other */) -> std::optional<bytes> {
                   return std::nullopt;
                 },
               },
               path.leaf_node.content);

  // If there are no nodes to hash, then ParentHash MUST be omitted
  if (hash_chain.empty()) {
    return !leaf_ph;
  }

  return leaf_ph && opt::get(leaf_ph) == hash_chain[0];
}

bool
TreeKEMPublicKey::exists_in_tree(const HPKEPublicKey& key,
                                 std::optional<LeafIndex> except) const
{
  return any_leaf([&](auto i, const auto& node) {
    return i != except && node.encryption_key == key;
  });
}

bool
TreeKEMPublicKey::exists_in_tree(const SignaturePublicKey& key,
                                 std::optional<LeafIndex> except) const
{
  return any_leaf([&](auto i, const auto& node) {
    return i != except && node.signature_key == key;
  });
}

tls::ostream&
operator<<(tls::ostream& str, const TreeKEMPublicKey& obj)
{
  // Empty tree
  if (obj.size.val == 0) {
    return str << std::vector<OptionalNode>{};
  }

  LeafIndex cut = LeafIndex{ obj.size.val - 1 };
  while (cut.val > 0 && obj.node_at(cut).blank()) {
    cut.val -= 1;
  }

  const auto begin = obj.nodes.begin();
  const auto end = begin + NodeIndex(cut).val + 1;
  const auto view = std::vector<OptionalNode>(begin, end);
  return str << view;
}

tls::istream&
operator>>(tls::istream& str, TreeKEMPublicKey& obj)
{
  // Read the node list
  str >> obj.nodes;
  if (obj.nodes.empty()) {
    return str;
  }

  // Verify that the tree is well-formed and minimal
  if (obj.nodes.size() % 2 == 0) {
    throw ProtocolError("Malformed ratchet tree: even number of nodes");
  }

  if (obj.nodes.back().blank()) {
    throw ProtocolError("Ratchet tree does not use minimal encoding");
  }

  // Adjust the size value to fit the non-blank nodes
  obj.size.val = 1;
  while (NodeCount(obj.size).val < obj.nodes.size()) {
    obj.size.val *= 2;
  }

  // Add blank nodes to the end
  obj.nodes.resize(NodeCount(obj.size).val);

  // Verify the basic structure of the tree is sane
  for (size_t i = 0; i < obj.nodes.size(); i++) {
    if (obj.nodes[i].blank()) {
      continue;
    }

    const auto& node = opt::get(obj.nodes[i].node).node;
    auto at_leaf = (i % 2 == 0);
    auto holds_leaf = var::holds_alternative<LeafNode>(node);
    auto holds_parent = var::holds_alternative<ParentNode>(node);

    if (at_leaf && !holds_leaf) {
      throw InvalidParameterError("Parent node in leaf node position");
    }

    if (!at_leaf && !holds_parent) {
      throw InvalidParameterError("Leaf node in parent node position");
    }
  }

  return str;
}

} // namespace mlspp
