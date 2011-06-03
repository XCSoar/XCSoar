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
#include "Navigation/TracePoint.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Compiler.h"

#include <set>
#include <list>
#include <assert.h>

struct AIRCRAFT_STATE;

/**
 * This class uses a smart thinning algorithm to limit the number of items
 * in the store.  The thinning algorithm is an online type of Douglas-Peuker
 * algorithm such that candidates are removed by ranking based on the
 * loss of precision caused by removing that point.  The loss is measured
 * by the difference between the distances between neighbours with and without
 * the candidate point removed.  In this version, time differences is also a
 * secondary factor, such that thinning attempts to remove points such that,
 * for equal distance ranking, smaller time step details are removed first.
 */
class Trace : private NonCopyable
{
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
      if (x.point.time < y.point.time)
        return true;

      if (x.point.time > y.point.time)
        return false;

      return false;
    }

    struct DeltaRankOp {
      bool operator()(const TraceDelta *s1, const TraceDelta *s2) {
        return DeltaRank(*s1, *s2);
      }
    };

    typedef std::set<TraceDelta *, DeltaRankOp> List;
    typedef List::iterator iterator;
    typedef List::const_iterator const_iterator;
    typedef std::pair<iterator, iterator> neighbours;

    TracePoint point;

    unsigned elim_time;
    unsigned elim_distance;
    unsigned delta_distance;

    // these items are mutable only so we can get around the const
    // nature of set iterators.  They don't affect set ordering though,
    // so doing this is safe.
    mutable iterator prev;
    mutable iterator next;

    TraceDelta(const TracePoint &p)
      :point(p),
       elim_time(null_time), elim_distance(null_delta),
       delta_distance(0) {}

    TraceDelta(const TracePoint &p_last, const TracePoint &p,
               const TracePoint &p_next)
      :point(p),
       elim_time(time_metric(p_last, p, p_next)),
       elim_distance(distance_metric(p_last, p, p_next)),
       delta_distance(p.approx_dist(p_last))
    {
      assert(elim_distance != null_delta);
    }

    void update(const TracePoint &p_last, const TracePoint &p_next) {
      elim_time = time_metric(p_last, point, p_next);
      elim_distance = distance_metric(p_last, point, p_next);
      delta_distance = point.approx_dist(p_last);
    }

    unsigned delta_time() const {
      return point.time - (*prev)->point.time;
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

  struct ChronologicalList : public std::list<TraceDelta> {
    iterator find_reference(const TraceDelta &ref) {
      for (iterator i = begin(); i != end(); ++i) {
        const TraceDelta &current = *i;
        if (&current == &ref)
          return i;
      }

      return end();
    }

    void erase_reference(const TraceDelta &ref) {
      iterator i = find_reference(ref);
      assert(i != end());
      erase(i);
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
    void append(const TracePoint &tp, ChronologicalList &chronological_list) {
      chronological_list.push_back(tp);
      TraceDelta &td = chronological_list.back();
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
          TraceDelta::iterator new_back = update_delta(old_back,
                                                       chronological_list);
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
        const TraceDelta &tit = **it;
        if (tit.point.time >= min_time)
          v.push_back(tit.point);

        it = tit.next;
      } while ((*it)->next != it);
      v.push_back((*it)->point);
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
                                      ChronologicalList &chronological_list) {
      assert(it != list.end());

      const TraceDelta::neighbours ne =
        std::make_pair((*it)->prev, (*it)->next);
      if ((ne.second == it) || (ne.first == it))
        return it;

      TraceDelta &td = **it;
      td.update((*ne.first)->point, (*ne.second)->point);

      // erase old one
      list.erase(it);

      // insert new in sorted position
      it = merge_insert(td);
      link(ne.first, it);
      link(it, ne.second);

      assert(chronological_list.size() == list.size());
      return it;
    }

    /**
     * Erase a non-edge item from delta list and tree, updating
     * deltas in the process.  This invalidates the calling iterator.
     *
     * @param it Item to erase
     * @param tree Tree to remove from
     *
     */
    void erase_inside(TraceDelta::iterator it,
                      ChronologicalList &chronological_list) {
      assert(it != list.end());

      // now delete the item
      TraceDelta::neighbours ne = erase(it);
      assert (ne.first != ne.second);

      chronological_list.erase_reference(**it);

      // and update the deltas
      update_delta(ne.first, chronological_list);
      update_delta(ne.second, chronological_list);
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
      const TraceDelta::neighbours ne =
        std::make_pair((*it)->prev, (*it)->next);
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
      link((*it)->prev, (*it)->next);
      TraceDelta::iterator next = (*it)->next;
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
    bool erase_delta(const unsigned target_size,
                     ChronologicalList &chronological_list,
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

        if ((*candidate)->point.time < recent_time) {
          erase_inside(candidate, chronological_list);
          lsize--;
          candidate = begin(); // find new top
          modified = true;
        } else {
          ++candidate;
          // suppressed removal, skip it.
        }
      }
      assert(list.size() == chronological_list.size());
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
    bool erase_earlier_than(const unsigned p_time,
                            ChronologicalList &chronological_list) {
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
        if ((*it)->point.time < p_time) {
          chronological_list.erase_reference(**it);
          list.erase(it);
          modified = true;
        } else if ((*it)->point.time < start_time) {
          start_time = (*it)->point.time;
          new_start = it;
        }
        it = next;
      }
      // need to set deltas for first point, only one of these
      // will occur (have to search for this point)
      if (modified && !list.empty())
        erase_start(new_start);

      assert(chronological_list.size() == list.size());
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
      if ((*d_last)->point.time> t)
        return (*d_last)->point.time-t;

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
        if ((*i)->point.time < r)
          acc += (*i)->delta_distance;
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
        if ((*i)->point.time < r)
          acc += (*i)->delta_time();
      }
      if (counter)
        return acc / counter;

      return 0;
    }

  private:
    void insert(TraceDelta &td) {
      list.insert(&td);
    }

    /**
     * Update from-to links for time-succession
     *
     * @param from From item
     * @param to To item
     */
    static void link(TraceDelta::iterator from, TraceDelta::iterator to) {
      (*to)->prev = from;
      (*from)->next = to;
    }

    /**
     * Update start node (and neighbour) after min time pruning
     */
    void erase_start(TraceDelta::iterator i_start) {
      TraceDelta::iterator i_next = (*i_start)->next;
      bool last = ((*i_start)->next == i_start);
      TraceDelta &td_start = **i_start;
      list.erase(i_start);
      td_start.elim_distance = null_delta;
      td_start.elim_time = null_time;
      TraceDelta::iterator i_new = merge_insert(td_start);
      (*i_new)->prev = i_new;
      if (last)
        link(i_new, i_new);
      else
        link(i_new, i_next);
    }

    /**
     * Insert new delta list item in sorted position
     *
     * @param td Point to add to delta list
     *
     * @return Iterator to inserted position
     */
    TraceDelta::iterator merge_insert(TraceDelta &td) {
      return list.insert(&td).first;
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
        if ((*i)->point.time== p_time)
          return i;

      assert(0);
      return list.end();
    }
  };

  ChronologicalList chronological_list;
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

public:
  class const_iterator {
    friend class Trace;

    ChronologicalList::const_iterator iterator;

    const_iterator(ChronologicalList::const_iterator _iterator)
      :iterator(_iterator) {}

  public:
    const TracePoint &operator*() const {
      return iterator->point;
    }

    const_iterator &operator++() {
      ++iterator;
      return *this;
    }

    const_iterator &NextSquareRange(unsigned sq_resolution,
                                    const const_iterator &end) {
      const TracePoint &previous = iterator->point;
      while (true) {
        ++iterator;

        if (iterator == end.iterator ||
            iterator->point.approx_sq_dist(previous) >= sq_resolution)
          return *this;
      }
    }

    bool operator==(const const_iterator &other) const {
      return iterator == other.iterator;
    }

    bool operator!=(const const_iterator &other) const {
      return iterator != other.iterator;
    }
  };

  const_iterator begin() const {
    return chronological_list.begin();
  }

  const_iterator end() const {
    return chronological_list.end();
  }

  gcc_pure
  unsigned ProjectRange(const GeoPoint &location, fixed distance) const {
    return task_projection.project_range(location, distance);
  }
};

#endif
