/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Util/queue.hpp"
#include <assert.h>
#include "Compiler.h"

//#define ASTAR_TR1

#ifdef ASTAR_TR1
#include <tr1/unordered_map>
#else
#include <map>
#endif

#ifdef INSTRUMENT_TASK
extern long count_astar_links;
#endif

#define ASTAR_MINMAX_OFFSET 134217727

#define ASTAR_QUEUE_SIZE 1024

struct AStarPriorityValue
{
  /** Actual edge value */
  unsigned g;
  /** Heuristic cost to goal */
  unsigned h;

  AStarPriorityValue(unsigned _g = 0):g(_g), h(0) {}
  AStarPriorityValue(const unsigned _g, const unsigned _h):g(_g), h(_h) {}

  gcc_pure
  AStarPriorityValue Adjust(const bool is_min) const {
    return is_min ? *this : AStarPriorityValue(ASTAR_MINMAX_OFFSET - g,
                                               ASTAR_MINMAX_OFFSET - h);
  }

  gcc_pure
  unsigned f() const {
    return g + h;
  }

  gcc_pure
  AStarPriorityValue operator+(const AStarPriorityValue& other) const {
    AStarPriorityValue n(*this);
    n.g += other.g;
    n.h = other.h;
    return n;
  }

  gcc_pure
  bool operator>(const AStarPriorityValue& other) const {
    return g > other.g;
  }
};

/**
 * AStar search algorithm, based on Dijkstra algorithm
 * Modifications by John Wharington to track optimal solution
 * @see http://en.giswiki.net/wiki/Dijkstra%27s_algorithm
 */
template <class Node, bool m_min=true>
class AStar
{
#ifdef ASTAR_TR1
  typedef std::tr1::unordered_map<Node, AStarPriorityValue> node_value_map;
#else
  typedef std::map<Node, AStarPriorityValue> node_value_map;
#endif

  typedef typename node_value_map::iterator node_value_iterator;
  typedef typename node_value_map::const_iterator node_value_const_iterator;

#ifdef ASTAR_TR1
  typedef std::tr1::unordered_map<Node, Node> node_parent_map;
#else
  typedef std::map<Node, Node> node_parent_map;
#endif

  typedef typename node_parent_map::iterator node_parent_iterator;
  typedef typename node_parent_map::const_iterator node_parent_const_iterator;

  typedef std::pair<AStarPriorityValue, node_value_iterator> NodeValue;

  struct Rank: public std::binary_function<NodeValue, NodeValue, bool>
  {
    gcc_pure
    bool operator()(const NodeValue &x, const NodeValue &y) const {
      return x.first.f() > y.first.f();
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
  /**
   * Default constructor
   *
   * @param is_min Whether this algorithm will search for min or max distance
   */
  AStar(unsigned reserve_default = ASTAR_QUEUE_SIZE)
  {
    Reserve(reserve_default);
  }

  /**
   * Constructor
   *
   * @param n Node to start
   * @param is_min Whether this algorithm will search for min or max distance
   */
  AStar(const Node &node, unsigned reserve_default = ASTAR_QUEUE_SIZE)
  {
    Push(node, node, AStarPriorityValue(0));
    Reserve(reserve_default);
  }

  /**
   * Resets as if constructed afresh
   *
   * @param n Node to start
   */
  void Restart(const Node &node) {
    Clear();
    Push(node, node, AStarPriorityValue(0));
  }

  /** Clears the queues */
  void Clear() {
    // Clear the search queue
    while (!q.empty())
      q.pop();

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
  gcc_pure
  bool IsEmpty() const {
    return q.empty();
  }

  /**
   * Return size of queue
   *
   * @return Queue size in elements
   */
  gcc_pure
  unsigned QueueSize() const {
    return q.size();
  }

  /**
   * Return top element of queue for processing
   *
   * @return Node for processing
   */
  const Node &Pop() {
    cur = q.top().second;

    do // remove this item
      q.pop();
    while (!q.empty() && (q.top().first > q.top().second->second));
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
            const AStarPriorityValue &edge_value) {
#ifdef INSTRUMENT_TASK
    count_astar_links++;
#endif
    Push(node, parent, GetNodeValue(parent) + edge_value.Adjust(m_min));
    // note order of + here is important!
  }

  /**
   * Find best predecessor found so far to the specified node
   *
   * @param n Node as destination to find best predecessor for
   *
   * @return Predecessor node
   */
  gcc_pure
  Node GetPredecessor(const Node &node) const {
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
  void Reserve(unsigned size) {
    q.reserve(size);
  }

  /**
   * Obtain the value of this node (accumulated distance to this node)
   * Returns 0 on failure to find the node.
   */
  gcc_pure
  AStarPriorityValue GetNodeValue(const Node &node) const {
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
            const AStarPriorityValue &edge_value) {
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

    q.push(std::make_pair(edge_value, it));
  }

  void SetPredecessor(const Node &node, const Node &parent) {
    // Try to find the given node in the node_parent_map
    node_parent_iterator it = node_parents.find(node);
    if (it == node_parents.end())
      // first entry
      // If the node wasn't found
      // -> Insert a new node into the node_parent_map
      node_parents.insert(std::make_pair(node, parent));
    else
      // If the node was found
      // -> Replace the according parent node with the new one
      it->second = parent;
  }
};

#endif
