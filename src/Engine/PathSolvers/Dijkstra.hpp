/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef DIJKSTRA_HPP
#define DIJKSTRA_HPP

#include "Util/ReservablePriorityQueue.hpp"
#include "Compiler.h"

#define DIJKSTRA_MINMAX_OFFSET 134217727

/**
 * Dijkstra search algorithm.
 * Modifications by John Wharington to track optimal solution
 * @see http://en.giswiki.net/wiki/Dijkstra%27s_algorithm
 */
template<typename Node, typename MapTemplate>
class Dijkstra
{
public:
  struct Edge
  {
    Node parent;

    unsigned value;

    Edge(Node _parent, unsigned _value):parent(_parent), value(_value) {}
  };

  typedef typename MapTemplate::template Bind<Edge> EdgeMap;
  typedef typename EdgeMap::iterator edge_iterator;
  typedef typename EdgeMap::const_iterator edge_const_iterator;

private:
  struct Value
  {
    unsigned edge_value;

    edge_iterator iterator;

    Value(unsigned _edge_value, edge_iterator _iterator)
      :edge_value(_edge_value), iterator(_iterator) {}
  };

  struct Rank : public std::binary_function<Value, Value, bool> {
    gcc_pure
    bool operator()(const Value &x, const Value &y) const {
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
  unsigned current_value;

public:
  /**
   * Default constructor
   */
  Dijkstra() = default;

  Dijkstra(const Dijkstra &) = delete;

  /** 
   * Clears the queues
   */
  void Clear() {
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
  const EdgeMap &GetEdgeMap() const {
    return edges;
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
  unsigned GetQueueSize() const {
    return q.size();
  }

  /**
   * Hack to allow incremental / continuous runs, see
   * ContestDijkstra::AddIncrementalEdges().
   */
  void SetCurrentValue(unsigned value) {
    current_value = value;
  }

  /**
   * Return top element of queue for processing
   *
   * @return Node for processing
   */
  Node Pop() {
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
  bool Link(const Node node, const Node parent, unsigned edge_value) {
    return Push(node, parent, current_value + edge_value);
  }

  /**
   * Find best predecessor found so far to the specified node
   *
   * @param n Node as destination to find best predecessor for
   *
   * @return Predecessor node
   */
  gcc_pure
  Node GetPredecessor(const Node node) const {
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
  void Reserve(unsigned size) {
    q.reserve(size);
  }

  /**
   * Clear the queue and re-insert all known links.
   */
  void RestartQueue() {
    // Clear the search queue
    q.clear();

    for (const auto &i : edges)
      q.push(Value(i.second.value, i));
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
  bool Push(const Node node, const Node parent, unsigned edge_value = 0) {
    // Try to find the given node n in the EdgeMap
    edge_iterator it = edges.find(node);
    if (it == edges.end())
      // first entry
      // If the node wasn't found
      // -> Insert a new node
      it = edges.insert(std::make_pair(node, Edge(parent, edge_value))).first;
    else if (it->second.value > edge_value)
      // If the node was found and the new value is smaller
      // -> Replace the value with the new one
      it->second = Edge(parent, edge_value);
    else
      // If the node was found but the new value is higher or equal
      // -> Don't use this new leg
      return false;

    q.push(Value(edge_value, it));
    return true;
  }
};

#endif
