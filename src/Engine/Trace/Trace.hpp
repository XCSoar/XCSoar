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

#ifndef TRACE_HPP
#define TRACE_HPP

#include "Point.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/SliceAllocator.hpp"
#include "Util/Serial.hpp"
#include "Geo/Flat/TaskProjection.hpp"
#include "Compiler.h"

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>

#include <algorithm>

#include <assert.h>
#include <stdlib.h>

class TracePointVector;
class TracePointerVector;

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
  struct TraceDelta
    : boost::intrusive::set_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>,
      boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>> {

    /**
     * Function used to points for sorting by deltas.
     * Ranking is primarily by distance delta; for equal distances, rank by
     * time delta.
     * This is like a modified Douglas-Peuker algorithm
     */
    gcc_pure
    static bool DeltaRank(const TraceDelta &x, const TraceDelta &y) {
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
      if (x.point.IsOlderThan(y.point))
        return true;

      if (x.point.IsNewerThan(y.point))
        return false;

      return false;
    }

    struct DeltaRankOp {
      gcc_pure
      bool operator()(const TraceDelta &s1, const TraceDelta &s2) const {
        return DeltaRank(s1, s2);
      }
    };

    TracePoint point;

    unsigned elim_time;
    unsigned elim_distance;
    unsigned delta_distance;

    explicit TraceDelta(const TracePoint &p)
      :point(p),
       elim_time(null_time), elim_distance(null_delta),
       delta_distance(0) {}

    TraceDelta(const TracePoint &p_last, const TracePoint &p,
               const TracePoint &p_next)
      :point(p),
       elim_time(TimeMetric(p_last, p, p_next)),
       elim_distance(DistanceMetric(p_last, p, p_next)),
       delta_distance(p.FlatDistanceTo(p_last))
    {
      assert(elim_distance != null_delta);
    }

    /**
     * Is this the first or the last point?
     */
    bool IsEdge() const {
      return elim_time == null_time;
    }

    void Update(const TracePoint &p_last, const TracePoint &p_next) {
      elim_time = TimeMetric(p_last, point, p_next);
      elim_distance = DistanceMetric(p_last, point, p_next);
      delta_distance = point.FlatDistanceTo(p_last);
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
    static unsigned DistanceMetric(const TracePoint &last,
                                   const TracePoint &node,
                                   const TracePoint &next) {
      const int d_this = last.FlatDistanceTo(node) + node.FlatDistanceTo(next);
      const int d_rem = last.FlatDistanceTo(next);
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
    static unsigned TimeMetric(const TracePoint &last, const TracePoint &node,
                               const TracePoint &next) {
      return next.DeltaTime(last)
        - std::min(next.DeltaTime(node), node.DeltaTime(last));
    }
  };

  /* using multiset, not because we need multiple values (we don't),
     but to avoid set's overhead for duplicate elimination */
  typedef boost::intrusive::multiset<TraceDelta,
                                     boost::intrusive::compare<TraceDelta::DeltaRankOp>,
                                     boost::intrusive::constant_time_size<false>> DeltaList;

  typedef boost::intrusive::list<TraceDelta,
                                 boost::intrusive::constant_time_size<false>> ChronologicalList;

  SliceAllocator<TraceDelta, 128u> allocator;

  DeltaList delta_list;
  ChronologicalList chronological_list;
  unsigned cached_size;

  TaskProjection task_projection;

  const unsigned max_time;
  const unsigned no_thin_time;
  const unsigned max_size;
  const unsigned opt_size;

  unsigned average_delta_time;
  unsigned average_delta_distance;

  Serial append_serial, modify_serial;

  template<typename Alloc>
  struct Disposer {
    Alloc &alloc;

    void operator()(typename Alloc::pointer td) {
      alloc.destroy(td);
      alloc.deallocate(td, 1);
    }
  };

  template<typename Alloc>
  static Disposer<Alloc> MakeDisposer(Alloc &alloc) {
    return {alloc};
  }

  Disposer<decltype(allocator)> MakeDisposer() {
    return MakeDisposer(allocator);
  }

public:
  /**
   * Constructor.  Task projection is updated after first call to append().
   *
   * @param no_thin_time Time window in seconds in which points most recent
   * wont be trimmed
   * @param max_time Time window size (seconds), null_time for unlimited
   * @param max_size Maximum number of points that can be stored
   */
  explicit Trace(const unsigned no_thin_time = 0,
                 const unsigned max_time = null_time,
                 const unsigned max_size = 1000);

  ~Trace() {
    clear();
  }

protected:
  /**
   * Find recent time after which points should not be culled
   * (this is set to n seconds before the latest time)
   *
   * @param t Time window
   *
   * @return Recent time
   */
  gcc_pure
  unsigned GetRecentTime(const unsigned t) const;

  /**
   * Update delta values for specified item in the delta list and the
   * tree.  This repositions the item after into its sorted position.
   *
   * @param it Item to update
   * @param tree Tree containing leaf
   *
   * @return Iterator to updated item
   */
  void UpdateDelta(TraceDelta &td);

  /**
   * Erase a non-edge item from delta list and tree, updating
   * deltas in the process.  This Invalidates the calling iterator.
   *
   * @param it Item to erase
   * @param tree Tree to remove from
   *
   */
  void EraseInside(DeltaList::iterator it);

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
  bool EraseDelta(const unsigned target_size,
                  const unsigned recent = 0);

  /**
   * Erase elements older than specified time from delta and tree,
   * and update earliest item to become the new start
   *
   * @param p_time Time to remove
   * @param tree Tree to remove from
   *
   * @return True if items were erased
   */
  bool EraseEarlierThan(const unsigned p_time);

  /**
   * Erase elements more recent than specified time.  This is used to
   * work around slight time warps.
   */
  void EraseLaterThan(const unsigned min_time);

  /**
   * Update start node (and neighbour) after min time pruning
   */
  void EraseStart(TraceDelta &td_start);

public:
  /**
   * Add trace to internal store.  Call optimise() periodically
   * to balance tree for faster queries
   *
   * @param a new point; its "flat" (projected) location is ignored
   */
  void push_back(const TracePoint &point);

  /**
   * Clear the trace store
   */
  void clear();

  void EraseEarlierThan(double time) {
    EraseEarlierThan((unsigned)time);
  }

  void EraseLaterThan(double time) {
    EraseLaterThan((unsigned)time);
  }

  unsigned GetMaxSize() const {
    return max_size;
  }

  /**
   * Size of traces (in tree, not in temporary store) ---
   * must call optimise() before this for it to be accurate.
   *
   * @return Number of traces in tree
   */
  unsigned size() const {
    return cached_size;
  }

  /**
   * Whether traces store is empty
   *
   * @return True if no traces stored
   */
  bool empty() const {
    return cached_size == 0;
  }

  /**
   * Returns a #Serial that gets incremented when data gets appended
   * to the #Trace.
   *
   * It also gets incremented on any other change, which makes this
   * method useful for checking whether the object is unmodified since
   * the last call.
   */
  const Serial &GetAppendSerial() const {
    return append_serial;
  }

  /**
   * Returns a #Serial that gets incremented when iterators get
   * Invalidated (e.g. when the #Trace gets cleared or optimised).
   */
  const Serial &GetModifySerial() const {
    return modify_serial;
  }

  /** 
   * Retrieve a vector of trace points sorted by time
   * 
   * @param iov Vector of trace points (output)
   *
   */
  void GetPoints(TracePointVector& iov) const;

  /**
   * Retrieve a vector of trace points sorted by time
   */
  void GetPoints(TracePointerVector &v) const;

  /**
   * Update the given #TracePointVector after points were appended to
   * this object.  This must not be called after thinning has
   * occurred, see GetModifySerial().
   *
   * @return true if new points were added
   */
  bool SyncPoints(TracePointerVector &v) const;

  /**
   * Fill the vector with trace points, not before #min_time, minimum
   * resolution #min_distance.
   */
  void GetPoints(TracePointVector &v, unsigned min_time,
                 const GeoPoint &location, double resolution) const;

  const TracePoint &front() const {
    assert(!empty());

    return chronological_list.front().point;
  }

  const TracePoint &back() const {
    assert(!empty());

    return chronological_list.back().point;
  }

private:
  /**
   * Enforce the maximum duration, i.e. remove points that are too
   * old.  This will be called before a new point is added, therefore
   * the time stamp of the new point is passed to this method.
   *
   * This method is a no-op if no time window was configured.
   *
   * @param latest_time the latest time stamp which is/will be stored
   * in this trace
   */
  void EnforceTimeWindow(unsigned latest_time);

  /**
   * Helper function for Thin().
   */
  void Thin2();

  /**
   * Thin the trace: remove old and irrelevant points to make room for
   * more points.
   */
  void Thin();

  TraceDelta &GetFront() {
    assert(!empty());

    return chronological_list.front();
  }

  TraceDelta &GetBack() {
    assert(!empty());

    return chronological_list.back();
  }

  gcc_pure
  unsigned CalcAverageDeltaDistance(const unsigned no_thin) const;

  gcc_pure
  unsigned CalcAverageDeltaTime(const unsigned no_thin) const;

  static constexpr unsigned null_delta = 0 - 1;

public:
  static constexpr unsigned null_time = 0 - 1;

  unsigned GetAverageDeltaDistance() const {
    return average_delta_distance;
  }

  unsigned GetAverageDeltaTime() const {
    return average_delta_time;
  }

public:
  class const_iterator : public ChronologicalList::const_iterator {
    friend class Trace;

    const_iterator(ChronologicalList::const_iterator &&_iterator)
      :ChronologicalList::const_iterator(std::move(_iterator)) {}

  public:
    using ChronologicalList::const_iterator::iterator_category;
    using ChronologicalList::const_iterator::difference_type;
    typedef const TracePoint value_type;
    typedef const TracePoint *pointer;
    typedef const TracePoint &reference;

    const_iterator() = default;

    const TracePoint &operator*() const {
      const TraceDelta &td = ChronologicalList::const_iterator::operator*();
      return td.point;
    }

    const TracePoint *operator->() const {
      const TraceDelta &td = ChronologicalList::const_iterator::operator*();
      return &td.point;
    }

    const_iterator &NextSquareRange(unsigned sq_resolution,
                                    const const_iterator &end) {
      const TracePoint &previous = **this;
      while (true) {
        ++*this;

        if (*this == end)
          return *this;

        const TraceDelta &td = (const TraceDelta &)**this;
        if (td.point.FlatSquareDistanceTo(previous) >= sq_resolution)
          return *this;
      }
    }
  };

  const_iterator begin() const {
    return chronological_list.begin();
  }

  const_iterator end() const {
    return chronological_list.end();
  }

  const TaskProjection &GetProjection() const {
    return task_projection;
  }

  gcc_pure
  unsigned ProjectRange(const GeoPoint &location, double distance) const {
    return task_projection.ProjectRangeInteger(location, distance);
  }
};

#endif
