// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/ReservablePriorityQueue.hpp"

#define DIJKSTRA_MINMAX_OFFSET 134217727

/**
 * Dijkstra search algorithm.
 * Modifications by John Wharington to track optimal solution
 * @see http://en.giswiki.net/wiki/Dijkstra%27s_algorithm
 */
template<typename Node, typename MapTemplate, typename ValueType=unsigned>
class Dijkstra
{
public:
  using value_type = ValueType;

  struct Edge
  {
    Node parent;

    value_type value;

    constexpr Edge(Node _parent, value_type _value) noexcept
      :parent(_parent), value(_value) {}
  };

  using EdgeMap = typename MapTemplate::template Bind<Edge>;
  using edge_iterator = typename EdgeMap::iterator;
  using edge_const_iterator = typename EdgeMap::const_iterator;

private:
  struct Value
  {
    value_type edge_value;

    edge_iterator iterator;

    constexpr Value(value_type _edge_value, edge_iterator _iterator) noexcept
      :edge_value(_edge_value), iterator(_iterator) {}
  };

  struct Rank {
    [[gnu::pure]]
    constexpr bool operator()(const Value &x, const Value &y) const noexcept {
      return x.edge_value > y.edge_value;
    }
  };

  /**
   * Stores the predecessor and value of each node.  It is updated by
   * push(), if a value lower than the current one is found.
   */
  EdgeMap edges;

  /**
   * A sorted list of all possible node paths, lowest distance first.
   */
  reservable_priority_queue<Value, std::vector<Value>, Rank> q;

  /**
   * The value of the current edge, i.e. the one that was consumed by
   * pop().
   */
  value_type current_value;

public:
  /**
   * Default constructor
   */
  Dijkstra() noexcept {
    /* this is a kludge to prevent rehashing, because rehashing would
       invalidate all iterators stored inside the priority queue
       "q", and would thus lead to use-after-free crashes */
    // TODO this hard-codes the use of std::unordered_map
    // TODO come up with a better solution
    edges.reserve(4093);
    edges.max_load_factor(1e10);
  }

  Dijkstra(const Dijkstra &) = delete;
  Dijkstra &operator=(const Dijkstra &) = delete;

  /** 
   * Clears the queues
   */
  void Clear() noexcept {
    // Clear the search queue
    q.clear();

    // Clear EdgeMap
    edges.clear();

    current_value = 0;
  }

  /**
   * Return a reference to the current edge map.  This hack is needed
   * for "continuous" search, see
   * ContestDijkstra::AddIncrementalEdges().
   */
  const EdgeMap &GetEdgeMap() const noexcept {
    return edges;
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
  auto GetQueueSize() const noexcept {
    return q.size();
  }

  /**
   * Hack to allow incremental / continuous runs, see
   * ContestDijkstra::AddIncrementalEdges().
   */
  void SetCurrentValue(value_type value) noexcept {
    current_value = value;
  }

  /**
   * Return top element of queue for processing
   *
   * @return Node for processing
   */
  Node Pop() noexcept {
    edge_const_iterator cur(q.top().iterator);
    current_value = cur->second.value;

    do {
      q.pop();
    } while (!q.empty() && q.top().iterator->second.value < q.top().edge_value);

    return cur->first;
  }

  /**
   * Add an edge (node-node-distance) to the search
   *
   * @param n Destination node to add
   * @param pn Predecessor of destination node
   * @param e Edge distance
   * @return false if this link was worse than an existing one
   */
  bool Link(const Node node, const Node parent, value_type edge_value) noexcept {
    return Push(node, parent, current_value + edge_value);
  }

  /**
   * Find best predecessor found so far to the specified node
   *
   * @param n Node as destination to find best predecessor for
   *
   * @return Predecessor node
   */
  [[gnu::pure]]
  Node GetPredecessor(const Node node) const noexcept {
    // Try to find the given node in the node_parent_map
    edge_const_iterator it = edges.find(node);
    if (it == edges.end())
      // first entry
      // If the node wasn't found
      // -> Return the given node itself
      return node;
    else
      // If the node was found
      // -> Return the parent node
      return it->second.parent;
  }

  /**
   * Reserve queue size (if available)
   */
  void Reserve(std::size_t size) noexcept {
    q.reserve(size);
  }

  /**
   * Clear the queue and re-insert all known links.
   */
  void RestartQueue() noexcept {
    // Clear the search queue
    q.clear();

    for (const auto &i : edges)
      q.emplace(i.second.value, i);
  }

private:
  /**
   * Add node to search queue
   *
   * @param n Destination node to add
   * @param pn Previous node
   * @param e Edge distance (previous to this)
   * @return false if this link was worse than an existing one
   */
  bool Push(const Node node, const Node parent,
            value_type edge_value = {}) noexcept {
    // Try to find the given node n in the EdgeMap
    const auto [it, inserted] = edges.try_emplace(node, parent, edge_value);
    if (inserted) {
      // first entry
    } else if (it->second.value > edge_value)
      // If the node was found and the new value is smaller
      // -> Replace the value with the new one
      it->second = Edge(parent, edge_value);
    else
      // If the node was found but the new value is higher or equal
      // -> Don't use this new leg
      return false;

    q.emplace(edge_value, it);
    return true;
  }
};
