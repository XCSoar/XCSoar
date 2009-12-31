/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include <map>
#include <queue>
#include <assert.h>

#ifdef INSTRUMENT_TASK
extern long count_dijkstra_links;
#endif

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
  Dijkstra(const bool is_min=true):m_min(is_min) {}

/** 
 * Constructor
 * 
 * @param n Node to start
 * @param e Initial edge distance
 */
  Dijkstra(const Node &n, const bool is_min=true):
    m_min(is_min) { 
    push(n, n, 0); 
  }

/** 
 * Resets as if constructed afresh
 * 
 * @param n Node to start
 */
  void reset(const Node &n) {
    clear();
    push(n,n,0);
  };

  void clear() {
    while (!q.empty()) {
      q.pop();
    }
    p.clear();
    m.clear();
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
  unsigned dist() const { return (cur->second); }

/** 
 * Add an edge (node-node-distance) to the search 
 * 
 * @param n Destination node to add
 * @param pn Predecessor of destination node
 * @param e Edge distance
 */
  void link(const Node &n, const Node &pn, const unsigned &e=1) { 
#ifdef INSTRUMENT_TASK
    count_dijkstra_links++;
#endif
    push(n, pn, cur->second + minmax_dist(e)); 
  }

/** 
 * Find best predecessor found so far to the specified node
 * 
 * @param n Node as destination to find best predecessor for
 * 
 * @return Predecessor node
 */
  Node get_predecessor(const Node &n) const {
    IterCP it = p.find(n);
    if (it == p.end()) { // first entry
      return n;
    } else {
      return (it->second); 
    }
  }
  
private:

  unsigned minmax_dist(const unsigned d) const {
    return m_min? d:134217727-d;
  }

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
  typedef typename std::map<Node, Node>::const_iterator IterCP;

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
  const bool m_min;
};

#endif
