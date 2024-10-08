#include "mls/tree_math.h"
#include "mls/common.h"

#include <algorithm>

static const uint32_t one = 0x01;

static uint32_t
log2(uint32_t x)
{
  if (x == 0) {
    return 0;
  }

  uint32_t k = 0;
  while ((x >> k) > 0) {
    k += 1;
  }
  return k - 1;
}

namespace mlspp {

LeafCount::LeafCount(const NodeCount w)
{
  if (w.val == 0) {
    val = 0;
    return;
  }

  if ((w.val & one) == 0) {
    throw InvalidParameterError("Only odd node counts describe trees");
  }

  val = (w.val >> one) + 1;
}

LeafCount
LeafCount::full(const LeafCount n)
{
  auto w = uint32_t(1);
  while (w < n.val) {
    w <<= 1U;
  }
  return LeafCount{ w };
}

NodeCount::NodeCount(const LeafCount n)
  : UInt32(2 * (n.val - 1) + 1)
{
}

LeafIndex::LeafIndex(NodeIndex x)
  : UInt32(0)
{
  if (x.val % 2 == 1) {
    throw InvalidParameterError("Only even node indices describe leaves");
  }

  val = x.val >> 1; // NOLINT(hicpp-signed-bitwise)
}

NodeIndex
LeafIndex::ancestor(LeafIndex other) const
{
  auto ln = NodeIndex(*this);
  auto rn = NodeIndex(other);
  if (ln == rn) {
    return ln;
  }

  uint8_t k = 0;
  while (ln != rn) {
    ln.val = ln.val >> 1U;
    rn.val = rn.val >> 1U;
    k += 1;
  }

  const uint32_t prefix = ln.val << k;
  const uint32_t stop = (1U << uint8_t(k - 1));
  return NodeIndex{ prefix + (stop - 1) };
}

NodeIndex::NodeIndex(LeafIndex x)
  : UInt32(2 * x.val)
{
}

NodeIndex
NodeIndex::root(LeafCount n)
{
  if (n.val == 0) {
    throw std::runtime_error("Root for zero-size tree is undefined");
  }

  auto w = NodeCount(n);
  return NodeIndex{ (one << log2(w.val)) - 1 };
}

bool
NodeIndex::is_leaf() const
{
  return val % 2 == 0;
}

bool
NodeIndex::is_below(NodeIndex other) const
{
  auto lx = level();
  auto ly = other.level();
  return lx <= ly && (val >> (ly + 1) == other.val >> (ly + 1));
}

NodeIndex
NodeIndex::left() const
{
  if (is_leaf()) {
    return *this;
  }

  // The clang analyzer doesn't realize that is_leaf() assures that level >= 1
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  return NodeIndex{ val ^ (one << (level() - 1)) };
}

NodeIndex
NodeIndex::right() const
{
  if (is_leaf()) {
    return *this;
  }

  return NodeIndex{ val ^ (uint32_t(0x03) << (level() - 1)) };
}

NodeIndex
NodeIndex::parent() const
{
  auto k = level();
  return NodeIndex{ (val | (one << k)) & ~(one << (k + 1)) };
}

NodeIndex
NodeIndex::sibling() const
{
  return sibling(parent());
}

NodeIndex
NodeIndex::sibling(NodeIndex ancestor) const
{
  if (!is_below(ancestor)) {
    throw InvalidParameterError("Node is not below claimed ancestor");
  }

  auto l = ancestor.left();
  auto r = ancestor.right();

  if (is_below(l)) {
    return r;
  }

  return l;
}

std::vector<NodeIndex>
NodeIndex::dirpath(LeafCount n)
{
  if (val >= NodeCount(n).val) {
    throw InvalidParameterError("Request for dirpath outside of tree");
  }

  auto d = std::vector<NodeIndex>{};

  auto r = root(n);
  if (*this == r) {
    return d;
  }

  auto p = parent();
  while (p.val != r.val) {
    d.push_back(p);
    p = p.parent();
  }

  // Include the root except in a one-member tree
  if (val != r.val) {
    d.push_back(p);
  }

  return d;
}

std::vector<NodeIndex>
NodeIndex::copath(LeafCount n)
{
  auto d = dirpath(n);
  if (d.empty()) {
    return {};
  }

  // Prepend leaf; omit root
  d.insert(d.begin(), *this);
  d.pop_back();

  return stdx::transform<NodeIndex>(d, [](auto x) { return x.sibling(); });
}

uint32_t
NodeIndex::level() const
{
  if ((val & one) == 0) {
    return 0;
  }

  uint32_t k = 0;
  while (((val >> k) & one) == 1) {
    k += 1;
  }
  return k;
}

} // namespace mlspp
