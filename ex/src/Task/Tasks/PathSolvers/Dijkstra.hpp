#ifndef DIJKSTRA_HPP
#define DIJKSTRA_HPP

#include <map>
#include <queue>

/**
 * Dijkstra search algorithm.
 * From http://en.giswiki.net/wiki/Dijkstra%27s_algorithm
 * with modifications by John Wharington to track optimal solution 
 */
template <class Node> class Dijkstra {
public:
/** 
 * Default constructor
 * 
 */
  Dijkstra() {}

/** 
 * Constructor
 * 
 * @param n Node to start
 * @param e Initial edge distance
 */
  Dijkstra(const Node &n, const unsigned &e=0) { 
    push(n, n, e); 
  }

/** 
 * Resets as if constructed afresh
 * 
 * @param n Node to start
 */
  void reset(const Node &n) {
    while (!q.empty()) {
      q.pop();
    }
    push(n,n,0);
  };

/** 
 * Test whether queue is empty 
 *
 * @return True if no more nodes to search
 */  
  bool empty() const { return q.empty(); }

/** 
 * Return top element of queue for processing
 * 
 * @return Node for processing
 */
  const Node &pop() {
    cur = q.top().second;
    do q.pop();
    while (!q.empty() && q.top().second->second < q.top().first);
    return cur->first;
  }

/** 
 * Return total edge distances 
 *
 * @return Total edge distances so far
 */
  const unsigned &dist() const { return cur->second; }

/** 
 * Add an edge (node-node-distance) to the search 
 * 
 * @param n Destination node to add
 * @param pn Predecessor of destination node
 * @param e Edge distance
 */
  void link(const Node &n, const Node &pn, const unsigned &e=1) { 
    push(n, pn, cur->second + e); 
  }

/** 
 * Find best predecessor found so far to the specified node
 * 
 * @param n Node as destination to find best predecessor for
 * 
 * @return Predecessor node
 */
  Node get_predecessor(const Node &n) {
    IterP it = p.find(n);
    if (it == p.end()) { // first entry
      return n;
    } else {
      return (it->second); 
    }
  }
  
private:
/** 
 * Add node to search queue
 * 
 * @param n Destination node to add
 * @param pn Previous node
 * @param e Edge distance (previous to this)
 */
  void push(const Node &n, const Node &pn, const unsigned &e=0) {
    Iter it = m.find(n);
    if (it == m.end()) { // first entry
      it = m.insert(make_pair(n, e)).first;
      set_predecessor(n, pn);
    } else if (it->second > e) {
      it->second = e; // replace, it's bigger
      set_predecessor(n, pn);
    } else 
      return;
    q.push(make_pair(e, it));
  }

  void set_predecessor(const Node &n, const Node &pn) {
    IterP it = p.find(n);
    if (it == p.end()) { // first entry
      p.insert(make_pair(n, pn));
    } else {
      it->second = pn; 
    }
  }

  typedef typename std::map<Node, unsigned>::iterator Iter;
  typedef typename std::map<Node, Node>::iterator IterP;

  typedef std::pair<unsigned, Iter> Value;

  struct Rank : public std::binary_function<Value, Value, bool> {
    bool operator()(const Value& x, const Value& y) const {
      return x.first > y.first;
    }
  };

  std::map<Node, unsigned> m;

  std::map<Node, Node> p;

  std::priority_queue<Value, std::vector<Value>, Rank> q;

  Iter cur;
};

#endif
