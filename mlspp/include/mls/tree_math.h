#pragma once

#include <cstdint>
#include <tls/tls_syntax.h>
#include <vector>

// The below functions provide the index calculus for the tree
// structures used in MLS.  They are premised on a "flat"
// representation of a balanced binary tree.  Leaf nodes are
// even-numbered nodes, with the n-th leaf at 2*n.  Intermediate
// nodes are held in odd-numbered nodes.  For example, a 11-element
// tree has the following structure:
//
//                                              X
//                      X
//          X                       X                       X
//    X           X           X           X           X
// X     X     X     X     X     X     X     X     X     X     X
// 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f 10 11 12 13 14
//
// This allows us to compute relationships between tree nodes simply
// by manipulating indices, rather than having to maintain
// complicated structures in memory, even for partial trees.  (The
// storage for a tree can just be a map[int]Node dictionary or an
// array.)  The basic rule is that the high-order bits of parent and
// child nodes have the following relation:
//
//    01x = <00x, 10x>

namespace mlspp {

// Index types go in the overall namespace
// XXX(rlb@ipv.sx): Seems like this stuff can probably get
// simplified down a fair bit.
struct UInt32
{
  uint32_t val;

  UInt32()
    : val(0)
  {
  }

  explicit UInt32(uint32_t val_in)
    : val(val_in)
  {
  }

  TLS_SERIALIZABLE(val)
};

struct NodeCount;

struct LeafCount : public UInt32
{
  using UInt32::UInt32;
  explicit LeafCount(const NodeCount w);

  static LeafCount full(const LeafCount n);
};

struct NodeCount : public UInt32
{
  using UInt32::UInt32;
  explicit NodeCount(const LeafCount n);
};

struct NodeIndex;

struct LeafIndex : public UInt32
{
  using UInt32::UInt32;
  explicit LeafIndex(const NodeIndex x);
  bool operator<(const LeafIndex other) const { return val < other.val; }
  bool operator<(const LeafCount other) const { return val < other.val; }

  NodeIndex ancestor(LeafIndex other) const;
};

struct NodeIndex : public UInt32
{
  using UInt32::UInt32;
  explicit NodeIndex(const LeafIndex x);
  bool operator<(const NodeIndex other) const { return val < other.val; }
  bool operator<(const NodeCount other) const { return val < other.val; }

  static NodeIndex root(LeafCount n);

  bool is_leaf() const;
  bool is_below(NodeIndex other) const;

  NodeIndex left() const;
  NodeIndex right() const;
  NodeIndex parent() const;
  NodeIndex sibling() const;

  // Returns the sibling of this node "relative to this ancestor" -- the child
  // of `ancestor` that is not in the direct path of this node.
  NodeIndex sibling(NodeIndex ancestor) const;

  std::vector<NodeIndex> dirpath(LeafCount n);
  std::vector<NodeIndex> copath(LeafCount n);

  uint32_t level() const;
};

} // namespace mlspp
