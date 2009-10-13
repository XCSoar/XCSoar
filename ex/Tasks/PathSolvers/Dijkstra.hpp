#ifndef DIJKSTRA_HPP
#define DIJKSTRA_HPP

#include <map>
#include <queue>

template <class Node> class Dijkstra {
public:
  Dijkstra() {}
  Dijkstra(const Node &n, const unsigned &e=0) { 
    push(n, e); 
  }

  void reset(const Node &n) {
    while (!q.empty()) {
      q.pop();
    }
    push(n,0);
  };

  bool empty() const { return q.empty(); }

  void push(const Node &n, const unsigned &e=0) {
    Iter it = m.find(n);
    if (it == m.end()) 
      it = m.insert(make_pair(n, e)).first;
    else if (it->second > e) {
      it->second = e; // replace, it's bigger
    } else 
      return;
    q.push(make_pair(e, it));
  }

  const Node &pop() {
    cur = q.top().second;
    do q.pop();
    while (!q.empty() && q.top().second->second < q.top().first);
    return cur->first;
  }

  const unsigned &dist() const { return cur->second; }

  void link(const Node &n, const unsigned &e=1) { 
    push(n, cur->second + e); 
  }
  
private:
  typedef typename std::map<Node, unsigned>::iterator Iter;

  typedef std::pair<unsigned, Iter> Value;

  struct Rank : public std::binary_function<Value, Value, bool> {
    bool operator()(const Value& x, const Value& y) const {
      return x.first > y.first;
    }
  };

  std::map<Node, unsigned> m;

  std::priority_queue<Value, std::vector<Value>, Rank> q;

  Iter cur;
};

#endif
