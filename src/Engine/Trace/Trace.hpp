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

#ifndef TRACE_HPP
#define TRACE_HPP

#include "Util/NonCopyable.hpp"
#include "Util/SliceAllocator.hpp"
#include <kdtree++/kdtree.hpp>
#include "Navigation/TracePoint.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Compiler.h"

#include <set>
#include <list>
#include <assert.h>

struct AIRCRAFT_STATE;

typedef std::set<TracePoint, TracePoint::time_sort> TracePointSet;
typedef std::list<TracePoint> TracePointList;

/**
 * Container for traces using kd-tree representation internally for fast 
 * geospatial lookups.
 *
 * This also uses a smart thinning algorithm to limit the number of items
 * in the store.  The thinning algorithm is an online type of Douglas-Peuker
 * algorithm such that candidates are removed by ranking based on the
 * loss of precision caused by removing that point.  The loss is measured
 * by the difference between the distances between neighbours with and without
 * the candidate point removed.  In this version, time differences is also a
 * secondary factor, such that thinning attempts to remove points such that,
 * for equal distance ranking, smaller time step details are removed first.
 *
 * The implementation uses a set/list to maintain a sorted list of
 * candidates and their ranking criteria such that sorting and
 * recalculation of their rank is not required unless the node or its
 * neighbours are deleted.  The list contains pointers to previous and
 * next (in time) items to avoid having to search the list for
 * neighbour lookups.  On deletion of items, the candidate list is
 * updated.
 *
 * The sorting ensures the last two points in the list are the earliest and
 * latest points, above these are candidates with most available to be removed
 * towards the top.
 *
 * Complexity analysis:
 *
 *   n: size of tree
 *
 * Insertion:
 *   O(log(n)) for set + O(log(n)) for kdtree
 *   = O(log(n))
 *
 * Erasing an item
 *   O(1) for set + O(log(n)) for kdtree + O(log(n)) neighbours
 *   = O(log(n))
 *
 * Pruning:
 *   O(n) erasures
 *   + O(n log^2 n) tree optimisation
 *   + O(n2) leaf reassignment
 *   = O(n + n2 + n log^2 n) per n steps
 *   = O(n + log^2 n)
 *
 * Retrieval of trace in time sequence:
 *   O(n)
 *
 * Geospatial find within range (k items within range):
 *   O(sqrt(n) + k)
 *
 * Conclusion:
 *   main overhead is due to leaf reassignment and tree construction;
 *   potential benefits if kdtree is modified to use a median-finding algorithm,
 *   and only updates parts of the tree it needs to rather than constructing
 *   afresh each time.
 *
 *   With current solution, we are using O(n + log^2 n) operations extra to save
 *   O(n) - O(sqrt(n)) operations in range searches.
 *
 *   Total cost
 *      O(n + log^2 n) + O(sqrt(n)) < O(n) ? no!
 *   But first part is done in idle time, so real question is:
 *      O(sqrt(n)) < O(n) ? yes!
 *
 *   Therefore, current solution is superior to not using kdtree.
 */
class Trace: private NonCopyable 
{
  friend class PrintHelper;

  /**
   * Type of KD-tree data structure for trace container
   */
  typedef KDTree::KDTree<2, TracePoint, TracePoint::kd_get_location,
                         KDTree::squared_difference<TracePoint::kd_get_location::result_type,
                                                    TracePoint::kd_get_location::result_type>,
                         std::less<TracePoint::kd_get_location::result_type>,
                         SliceAllocator<KDTree::_Node<TracePoint>, 256> > TraceTree;

  struct TraceDelta {
    /**
     * Function used to points for sorting by deltas.
     * Ranking is primarily by distance delta; for equal distances, rank by
     * time delta.
     * This is like a modified Douglas-Peuker algorithm
     */
    static bool DeltaRank (const TraceDelta& x, const TraceDelta& y) {
      // distance is king
      if (x.elim_distance < y.elim_distance)
        return true;

      if (x.elim_distance > y.elim_distance)
        return false;

      // distance is equal, so go by time error
      if (x.elim_time < y.elim_time)
        return true;

      if (x.elim_time > y.elim_time)
        return false;

      // all else fails, go by age
      if (x.p_time < y.p_time)
        return true;

      if (x.p_time > y.p_time)
        return false;

      return false;
    }

    struct DeltaRankOp {
      bool operator()(const TraceDelta& s1, const TraceDelta& s2) {
        return DeltaRank(s1, s2);
      }
    };

    typedef std::set<TraceDelta, DeltaRankOp> List;
    typedef List::iterator iterator;
    typedef List::const_iterator const_iterator;
    typedef std::pair<iterator, iterator> neighbours;

    unsigned p_time;
    unsigned elim_time;
    unsigned elim_distance;
    unsigned delta_distance;

    // these items are mutable only so we can get around the const
    // nature of set iterators.  They don't affect set ordering though,
    // so doing this is safe.
    mutable TraceTree::const_iterator leaf;
    mutable iterator prev;
    mutable iterator next;

    TraceDelta(const TracePoint &p, const TraceTree::const_iterator &_leaf)
    {
      p_time = p.time;
      leaf = _leaf;
      elim_distance = null_delta;
      elim_time = null_time;
      delta_distance = 0;
    }

    TraceDelta(const TracePoint& p_last, const TracePoint& p,
               const TracePoint& p_next, const TraceTree::const_iterator &_leaf)
    {
      p_time = p.time;
      elim_distance = distance_metric(p_last, p, p_next);
      elim_time = time_metric(p_last, p, p_next);
      delta_distance = p.approx_dist(p_last);
      leaf = _leaf;
      assert (elim_distance != null_delta);
    }

    void set_times(const TracePoint &p) {
      p_time = p.time;
    }

    unsigned delta_time() const {
      return p_time - prev->p_time;
    }

    /**
     * Calculate error distance, between last through this to next,
     * if this node is removed.  This metric provides for Douglas-Peuker
     * thinning.
     *
     * @param last Point previous in time to this node
     * @param node This node
     * @param next Point succeeding this node
     *
     * @return Distance error if this node is thinned
     */
    static unsigned distance_metric(const TracePoint& last,
                                    const TracePoint& node,
                                    const TracePoint& next) {
      const int d_this = last.approx_dist(node) + node.approx_dist(next);
      const int d_rem = last.approx_dist(next);
      return abs(d_this - d_rem);
    }

    /**
     * Calculate error time, between last through this to next,
     * if this node is removed.  This metric provides for fair thinning
     * (tendency to to result in equal time steps)
     *
     * @param last Point previous in time to this node
     * @param node This node
     * @param next Point succeeding this node
     *
     * @return Time delta if this node is thinned
     */
    static unsigned time_metric(const TracePoint& last, const TracePoint& node,
                                const TracePoint& next) {
      return (next.time - last.time) -
             std::min(next.time - node.time, node.time - last.time);
    }
  };

  class DeltaList {
    TraceDelta::List list;

  public:
    /**
     * Append new point to tree and list in sorted order.
     * It is expected this will be called in increasing time order,
     * if not, the system should be cleared.
     *
     * @param tp Point to add
     * @param tree Tree to store items
     */
    void append(const TracePoint &tp, TraceTree& tree) {
      TraceDelta td(tp, tree.insert(tp));
      if (list.empty()) {
        insert(td); // will always be at back
        TraceDelta::iterator back = list.begin();
        link(back, back);
      } else {
        TraceDelta::iterator old_back = last();
        insert(td);
        TraceDelta::iterator new_it = last();
        link(new_it, new_it);
        link(old_back, new_it);
        if (list.size() > 2) {
          TraceDelta::iterator new_back = update_delta(old_back, tree);
          link(new_back, new_it);
        }
      }
    }

    void clear() {
      list.clear();
    }

    // return top element of list
    TraceDelta::iterator begin() {
      return list.begin();
    }

    unsigned size() const {
      return list.size();
    }

    void get_trace(TracePointVector &v, const unsigned min_time = 0) const {
      v.clear();

      if (list.size() < 3)
        return;

      v.reserve(list.size());
      TraceDelta::const_iterator it = first();
      assert(it != last());
      do {
        const TraceDelta &tit = *it;
        if (tit.p_time >= min_time)
          v.push_back(*(tit.leaf));

        it = tit.next;
      } while (it->next != it);
      v.push_back(*(it->leaf));
    }

    /**
     * Update delta values for specified item in the delta list and the
     * tree.  This repositions the item after into its sorted position.
     *
     * @param it Item to update
     * @param tree Tree containing leaf
     *
     * @return Iterator to updated item
     */
    TraceDelta::iterator update_delta(TraceDelta::iterator it,
                                      TraceTree& tree) {
      assert(it != list.end());

      const TraceDelta::neighbours ne = std::make_pair(it->prev, it->next);
      if ((ne.second == it) || (ne.first == it))
        return it;

      TraceDelta td (*(ne.first->leaf), *it->leaf, *(ne.second->leaf), it->leaf);

      // erase old one
      list.erase(it);

      // insert new in sorted position
      it = merge_insert(td);
      link(ne.first, it);
      link(it, ne.second);

      merge_leaf(it, tree);

      assert(tree.size() == list.size());
      return it;
    }

    /**
     * Update links from tree leaves to the delta list.
     * This is slow but necessary since leaf iterators are invalidated
     * after optimise.
     *
     * @param tree Tree to point to
     */
    void update_leaves(const TraceTree& tree) {
      TraceTree::const_iterator tend = tree.end();
      for (TraceTree::const_iterator tr = tree.begin();
           tr != tend; ++tr) {
        TraceDelta::iterator id = find_time(tr->time);
        id->leaf = tr;
      }
    }

    /**
     * Erase a non-edge item from delta list and tree, updating
     * deltas in the process.  This invalidates the calling iterator.
     *
     * @param it Item to erase
     * @param tree Tree to remove from
     *
     */
    void erase_inside(TraceDelta::iterator it, TraceTree& tree) {
      assert(it != list.end());

      // now delete the item
      tree.erase_exact(*it->leaf);

      TraceDelta::neighbours ne = erase(it);
      assert (ne.first != ne.second);

      // and update the deltas
      update_delta(ne.first, tree);
      update_delta(ne.second, tree);
    }

    /**
     * Erase element and update time link, returning neighbours.
     * must update the distance delta of both neighbours after!
     *
     * @param it Item to erase
     *
     * @return Neighbours
     */
    TraceDelta::neighbours erase(TraceDelta::iterator it) {
      assert(it != list.end());
      const TraceDelta::neighbours ne = std::make_pair(it->prev, it->next);
      assert(ne.first != it);
      assert(ne.second != it);

      // next and last now linked
      list.erase(it);
      link(ne.first, ne.second);
      return ne;
    }

    /**
     * Erase element from list, without updating deltas.
     * For when this class is used as a conventionally sorted list
     * (increasing time order)
     */
    TraceDelta::iterator erase_from_list(TraceDelta::iterator it) {
      link(it->prev, it->next);
      TraceDelta::iterator next = it->next;
      list.erase(it);
      return next;
    }

    /**
     * Erase elements based on delta metric until the size is
     * equal to the target size.  Wont remove elements more recent than
     * specified time from the last point.
     *
     * Note that the recent time is obeyed even if the results will
     * fail to set the target size.
     *
     * @param target_size Size of desired list.
     * @param tree Tree to remove from
     * @param recent Time window for which to not remove points
     *
     * @return True if items were erased
     */
    bool erase_delta(const unsigned target_size, TraceTree& tree,
                     const unsigned recent = 0) {
      if (size() < 2)
        return false;

      bool modified = false;

      const unsigned recent_time = get_recent_time(recent);
      unsigned lsize = list.size();

      TraceDelta::const_iterator lfirst = first();
      TraceDelta::iterator candidate = begin();
      while (lsize > target_size) {
        if (candidate == lfirst)
          // exhausted non-recent candidates!
          return modified;

        if (candidate->p_time < recent_time) {
          erase_inside(candidate, tree);
          lsize--;
          candidate = begin(); // find new top
          modified = true;
        } else {
          ++candidate;
          // suppressed removal, skip it.
        }
      }
      assert(list.size() == tree.size());
      return modified;
    }

    /**
     * Erase elements older than specified time from delta and tree,
     * and update earliest item to become the new start
     *
     * @param p_time Time to remove
     * @param tree Tree to remove from
     *
     * @return True if items were erased
     */
    bool erase_earlier_than(const unsigned p_time, TraceTree& tree) {
      if (!p_time)
        // there will be nothing to remove
        return false;

      bool modified = false;
      unsigned start_time = null_time;
      TraceDelta::iterator new_start = list.begin();

      for (TraceDelta::iterator it = list.begin();
           it != list.end(); ) {
        TraceDelta::iterator next = it; 
        ++next;
        if (it->p_time < p_time) {
          tree.erase(it->leaf);
          list.erase(it);
          modified = true;
        } else if (it->p_time < start_time) {
          start_time = it->p_time;
          new_start = it;
        }
        it = next;
      }
      // need to set deltas for first point, only one of these
      // will occur (have to search for this point)
      if (modified && !list.empty())
        erase_start(new_start);

      assert(tree.size() == list.size());
      return modified;
    }

    /**
     * Find recent time after which points should not be culled
     * (this is set to n seconds before the latest time)
     *
     * @param t Time window
     *
     * @return Recent time
     */
    unsigned get_recent_time(const unsigned t) const {
      if (empty())
        return 0;

      TraceDelta::const_iterator d_last = last();
      if (d_last->p_time> t)
        return d_last->p_time-t;

      return 0;
    }

    /**
     * Find latest item in list.  Returns end() on failure.
     *
     * @return Iterator to last (latest) item
     */
    gcc_pure
    TraceDelta::iterator last() {
      assert(list.size());
      if (!list.empty()) {
        TraceDelta::iterator i = list.end();
        --i;
        return i;
      }

      return list.end();
    }

    /**
     * Find latest item in list.  Returns end() on failure.
     *
     * @return Iterator to last (latest) item
     */
    gcc_pure
    TraceDelta::const_iterator last() const {
      assert(list.size());
      if (!list.empty()) {
        TraceDelta::const_iterator i = list.end();
        --i;
        return i;
      }

      return list.end();
    }

    /**
     * Find first item in list (by time).  Returns end() on failure
     *
     * @return Iterator to first (earliest) item
     */
    gcc_pure
    TraceDelta::iterator first() {
      if (size() > 1) {
        TraceDelta::iterator i = last();
        --i;
        return i;
      } else {
        assert(1);
        return list.end();
      }
    }

    /**
     * Find first item in list (by time).  Returns end() on failure
     *
     * @return Iterator to first (earliest) item
     */
    gcc_pure
    TraceDelta::const_iterator first() const {
      if (size() > 1) {
        TraceDelta::const_iterator i = last();
        --i;
        return i;
      } else {
        assert(1);
        return list.end();
      }
    }

    /**
     * Determine if delta list is empty
     *
     * @return True if list is empty
     */
    bool empty() const {
      return list.empty();
    }

    unsigned calc_average_delta_distance(const unsigned no_thin) const {
      unsigned r = get_recent_time(no_thin);
      unsigned acc = 0;
      unsigned counter = 0;
      for (TraceDelta::const_iterator i= list.begin();
           i!= list.end(); ++i, ++counter) {
        if (i->p_time < r)
          acc += i->delta_distance;
      }
      if (counter)
        return acc / counter;

      return 0;
    }

    unsigned calc_average_delta_time(const unsigned no_thin) const {
      unsigned r = get_recent_time(no_thin);
      unsigned acc = 0;
      unsigned counter = 0;
      for (TraceDelta::const_iterator i= list.begin();
           i!= list.end(); ++i, ++counter) {
        if (i->p_time < r)
          acc += i->delta_time();
      }
      if (counter)
        return acc / counter;

      return 0;
    }

  private:
    void insert(const TraceDelta &td) {
      list.insert(td);
    }

    /**
     * Update from-to links for time-succession
     *
     * @param from From item
     * @param to To item
     */
    static void link(TraceDelta::iterator from, TraceDelta::iterator to) {
      to->prev = from;
      from->next = to;
    }

    /**
     * Update start node (and neighbour) after min time pruning
     */
    void erase_start(TraceDelta::iterator i_start) {
      TraceDelta::iterator i_next = i_start->next;
      bool last = (i_start->next == i_start);
      TraceDelta td_start = *i_start;
      list.erase(i_start);
      td_start.elim_distance = null_delta;
      td_start.elim_time = null_time;
      TraceDelta::iterator i_new = merge_insert(td_start);
      i_new->prev = i_new;
      if (last)
        link(i_new, i_new);
      else
        link(i_new, i_next);
    }

    /**
     * Create new tree leaf with corrected time links due to removal of
     * its predecessor in time.
     *
     * @param it Item for which leaf must be updated
     * @param tree Tree structure to work on
     */
    void merge_leaf(TraceDelta::iterator it, TraceTree& tree) {
      // @todo merge data for erased point?
      it->leaf->last_time = it->prev->p_time;
    }

    /**
     * Insert new delta list item in sorted position
     *
     * @param td Point to add to delta list
     *
     * @return Iterator to inserted position
     */
    TraceDelta::iterator merge_insert(const TraceDelta td) {
      return list.insert(td).first;
    }

    /**
     * Finds item of set time. Undefined behaviour if not found.
     *
     * @param p_time Time to search for
     *
     * @return Iterator to found item
     */
    TraceDelta::iterator find_time(const unsigned p_time) {
      TraceDelta::const_iterator lend = list.end();
      for (TraceDelta::iterator i= list.begin(); i!= lend; ++i)
        if (i->p_time== p_time)
          return i;

      assert(0);
      return list.end();
    }
  };

  TraceTree trace_tree;
  DeltaList delta_list;
  TaskProjection task_projection;
  TracePoint m_last_point;

  const unsigned m_max_time;
  const unsigned no_thin_time;
  const unsigned m_max_points;
  const unsigned m_opt_points;

  unsigned m_average_delta_time;
  unsigned m_average_delta_distance;

public:
  /**
   * Constructor.  Task projection is updated after first call to append().
   *
   * @param no_thin_time Time window in seconds in which points most recent
   * wont be trimmed
   * @param max_time Time window size (seconds), null_time for unlimited
   * @param max_points Maximum number of points that can be stored
   */
  Trace(const unsigned no_thin_time = 0,
        const unsigned max_time = null_time,
        const unsigned max_points = 1000);

  /**
   * Add trace to internal store.  Call optimise() periodically
   * to balance tree for faster queries
   *
   * @param state Aircraft state to log point for
   */
  void append(const AIRCRAFT_STATE& state);

  /**
   * Clear the trace store
   */
  void clear();

  /**
   * Size of traces (in tree, not in temporary store) ---
   * must call optimise() before this for it to be accurate.
   *
   * @return Number of traces in tree
   */
  unsigned size() const;

  /**
   * Whether traces store is empty
   *
   * @return True if no traces stored
   */
  bool empty() const;

  /**
   * Re-balance kd-tree periodically 
   *
   * @return True if trace store was optimised
   */
  bool optimise_if_old();

  /**
   * Find traces within approximate range (square range box)
   * to search location.  Possible use by screen display functions.
   *
   * @param loc Location from which to search
   * @param range Distance in meters of search radius
   * @param mintime Minimum time to match (recency)
   * @param resolution Thin data to achieve minimum step size in (m) (if positive)
   *
   * @return Vector of trace points within square range
   */
  gcc_pure
  TracePointVector
  find_within_range(const GeoPoint &loc, const fixed range,
                    const unsigned mintime = 0,
                    const fixed resolution = fixed_zero) const;

  /** 
   * Retrieve a vector of trace points sorted by time
   * 
   * @param edges_only Search only for trace edges (first, last time)
   * @param iov Vector of trace points (output)
   *
   */
  void get_trace_points(TracePointVector& iov) const {
    if (delta_list.size() < 3)
      get_trace_edges(iov);
    else
      delta_list.get_trace(iov, get_min_time());
  }

  /**
   * Retrieve a vector of the earliest and latest trace points
   * 
   * @param iov Vector of trace points
   */
  void get_trace_edges(TracePointVector& iov) const;

private:
  static void thin_trace_resolution(TracePointList& vec, const unsigned range_sq);

  gcc_pure
  bool inside_time_window(unsigned time) const;

  gcc_pure
  unsigned get_min_time() const;

  static const unsigned null_delta = 0 - 1;

public:
  static const unsigned null_time = 0 - 1;

  unsigned is_recent(const TracePoint& p) const {
    return m_last_point.time - p.time > delta_list.get_recent_time(no_thin_time);
  }

  unsigned average_delta_distance() const {
    return m_average_delta_distance;
  }

  unsigned average_delta_time() const {
    return m_average_delta_time;
  }

  /**
   * Check if this point is invalid
   *
   * @return True if point is invalid
   */
  gcc_pure
  static bool is_null(const TracePoint& tp);

  /**
   * Get last point added to store
   *
   * @return Last point added
   */
  gcc_pure
  const TracePoint& get_last_point() const {
    return m_last_point;
  }
};

#endif
