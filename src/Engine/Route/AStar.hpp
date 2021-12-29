/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef ASTAR_HPP
#define ASTAR_HPP

#include "util/ReservablePriorityQueue.hpp"

#include <unordered_map>

struct AStarPriorityValue
{
  static constexpr unsigned MINMAX_OFFSET = 134217727;

  /** Actual edge value */
  unsigned g;
  /** Heuristic cost to goal */
  unsigned h;

  explicit constexpr AStarPriorityValue(unsigned _g) noexcept:g(_g), h(0) {}

  constexpr AStarPriorityValue(const unsigned _g, const unsigned _h) noexcept
    :g(_g), h(_h) {}

  template<bool is_min>
  constexpr
  AStarPriorityValue Adjust() const noexcept {
    return is_min ? *this : AStarPriorityValue(MINMAX_OFFSET - g,
                                               MINMAX_OFFSET - h);
  }

  constexpr unsigned f() const noexcept {
    return g + h;
  }

  constexpr
  AStarPriorityValue operator+(const AStarPriorityValue &other) const noexcept {
    return AStarPriorityValue(g + other.g, other.h);
  }

  constexpr
  bool operator>(const AStarPriorityValue &other) const noexcept {
    return g > other.g;
  }
};

/**
 * AStar search algorithm, based on Dijkstra algorithm
 * Modifications by John Wharington to track optimal solution
 * @see http://en.giswiki.net/wiki/Dijkstra%27s_algorithm
 */
template <class Node, class Hash=std::hash<Node>,
          class KeyEqual=std::equal_to<Node>,
          bool m_min=true>
class AStar
{
  typedef std::unordered_map<Node, AStarPriorityValue, Hash, KeyEqual> node_value_map;

  typedef typename node_value_map::iterator node_value_iterator;
  typedef typename node_value_map::const_iterator node_value_const_iterator;

  typedef std::unordered_map<Node, Node, Hash, KeyEqual> node_parent_map;

  typedef typename node_parent_map::iterator node_parent_iterator;
  typedef typename node_parent_map::const_iterator node_parent_const_iterator;

  struct NodeValue {
    AStarPriorityValue priority;

    node_value_iterator iterator;

    constexpr
    NodeValue(const AStarPriorityValue &_priority,
              node_value_iterator _iterator) noexcept
      :priority(_priority), iterator(_iterator) {}
  };

  struct Rank {
    constexpr
    bool operator()(const NodeValue &x, const NodeValue &y) const noexcept {
      return x.priority.f() > y.priority.f();
    }
  };

  /**
   * Stores the value of each node.  It is updated by push(), if a
   * value lower than the current one is found.
   */
  node_value_map node_values;

  /**
   * Stores the predecessor of each node.  It is maintained by
   * set_predecessor().
   */
  node_parent_map node_parents;

  /**
   * A sorted list of all possible node paths, lowest distance first.
   */
  reservable_priority_queue<NodeValue, std::vector<NodeValue>, Rank> q;

  node_value_iterator cur;

public:
  static constexpr unsigned DEFAULT_QUEUE_SIZE = 1024;

  /**
   * Default constructor
   *
   * @param is_min Whether this algorithm will search for min or max distance
   */
  AStar(unsigned reserve_default = DEFAULT_QUEUE_SIZE) noexcept
  {
    Reserve(reserve_default);
  }

  /**
   * Constructor
   *
   * @param n Node to start
   * @param is_min Whether this algorithm will search for min or max distance
   */
  AStar(const Node &node,
        unsigned reserve_default = DEFAULT_QUEUE_SIZE) noexcept
  {
    Reserve(reserve_default);
    Push(node, node, AStarPriorityValue(0));
  }

  /**
   * Resets as if constructed afresh
   *
   * @param n Node to start
   */
  void Restart(const Node &node) noexcept {
    Clear();
    Push(node, node, AStarPriorityValue(0));
  }

  /** Clears the queues */
  void Clear() noexcept {
    // Clear the search queue
    q.clear();

    // Clear the node_parent_map
    node_parents.clear();
    // Clear the node_value_map
    node_values.clear();
  }

  /**
   * Test whether queue is empty
   *
   * @return True if no more nodes to search
   */
  [[gnu::pure]]
  bool IsEmpty() const noexcept {
    return q.empty();
  }

  /**
   * Return size of queue
   *
   * @return Queue size in elements
   */
  [[gnu::pure]]
  unsigned QueueSize() const noexcept {
    return q.size();
  }

  /**
   * Return top element of queue for processing
   *
   * @return Node for processing
   */
  const Node &Pop() noexcept {
    cur = q.top().iterator;

    do { // remove this item
      q.pop();
    } while (!q.empty() && (q.top().priority > q.top().iterator->second));
    // and all lower rank than this

    return cur->first;
  }

  /**
   * Add an edge (node-node-distance) to the search
   *
   * @param n Destination node to add
   * @param pn Predecessor of destination node
   * @param e Edge distance
   */
  void Link(const Node &node, const Node &parent,
            const AStarPriorityValue &edge_value) noexcept {
    Push(node, parent, GetNodeValue(parent) + edge_value.Adjust<m_min>());
    // note order of + here is important!
  }

  /**
   * Find best predecessor found so far to the specified node
   *
   * @param n Node as destination to find best predecessor for
   *
   * @return Predecessor node
   */
  [[gnu::pure]]
  Node GetPredecessor(const Node &node) const noexcept {
    // Try to find the given node in the node_parent_map
    node_parent_const_iterator it = node_parents.find(node);
    if (it == node_parents.end())
      // first entry
      // If the node wasn't found
      // -> Return the given node itself
      return node;

    // If the node was found
    // -> Return the parent node
    return it->second;
  }

  /** Reserve queue size (if available) */
  void Reserve(unsigned size) noexcept {
    q.reserve(size);
  }

  /**
   * Obtain the value of this node (accumulated distance to this node)
   * Returns 0 on failure to find the node.
   */
  [[gnu::pure]]
  AStarPriorityValue GetNodeValue(const Node &node) const noexcept {
    node_value_const_iterator it = node_values.find(node);
    if (cur->first == node)
      return cur->second;

    if (it == node_values.end())
      return AStarPriorityValue(0);

    return it->second;
  }

private:
  /**
   * Add node to search queue
   *
   * @param n Destination node to add
   * @param pn Previous node
   * @param e Edge distance (previous to this)
   */
  void Push(const Node &node, const Node &parent,
            const AStarPriorityValue &edge_value) noexcept {
    // Try to find the given node n in the node_value_map
    node_value_iterator it = node_values.find(node);
    if (it == node_values.end()) {
      // first entry
      // If the node wasn't found
      // -> Insert a new node into the node_value_map
      it = node_values.insert(std::make_pair(node, edge_value)).first;

      // Remember the parent node
      SetPredecessor(node, parent);
    } else if (it->second > edge_value) {
      // If the node was found and the new value is smaller
      // -> Replace the value with the new one
      it->second = edge_value;
      // replace, it's bigger

      // Remember the new parent node
      SetPredecessor(node, parent);
    } else
      // If the node was found but the value is higher or equal
      // -> Don't use this new leg
      return;

    q.push(NodeValue(edge_value, it));
  }

  void SetPredecessor(const Node &node, const Node &parent) noexcept {
    // Try to find the given node in the node_parent_map
    auto result = node_parents.insert(std::make_pair(node, parent));
    if (!result.second)
      // If the node was found
      // -> Replace the according parent node with the new one
      result.first->second = parent;
  }
};

#endif
