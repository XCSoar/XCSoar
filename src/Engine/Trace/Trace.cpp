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

#include "Trace.hpp"
#include "Navigation/Aircraft.hpp"
#include <algorithm>

Trace::Trace(const unsigned _no_thin_time, const unsigned max_time,
             const unsigned max_points)
  :m_max_time(max_time),
   no_thin_time(_no_thin_time),
   m_max_points(max_points),
   m_opt_points((3*max_points)/4)
{
  m_last_point.time = null_time;
  assert(max_points >= 4);
}

void
Trace::clear()
{
  trace_tree.clear();
  trace_tree.optimize();
  m_last_point.time = null_time;
  m_average_delta_distance = 0;
  m_average_delta_time = 0;
  delta_list.clear();
}

void
Trace::append(const AIRCRAFT_STATE& state)
{
  if (empty()) {
    // first point determines origin for flat projection
    task_projection.reset(state.get_location());
    task_projection.update_fast();
    m_last_point.time = null_time;
  } else if (trace_tree.size() > 0 && state.Time < fixed(m_last_point.time)) {
    // gone back in time, must reset. (shouldn't get here!)
    assert(1);
    clear();
    return;
  }

  TracePoint tp(state);
  tp.project(task_projection);

  // only add one item per two seconds
  if ((tp.time - m_last_point.time) < 2)
    return;

  tp.project(task_projection);
  tp.last_time = m_last_point.time;
  m_last_point = tp;

  delta_list.append(tp, trace_tree);
}

unsigned
Trace::get_min_time() const
{
  if (m_last_point.time == null_time ||
      m_max_time == null_time)
    return 0;

  return std::max(0, (int)m_last_point.time - (int)m_max_time);
}

bool
Trace::optimise_if_old()
{
  if (trace_tree.size() >= m_max_points) {
    // first remove points outside max time range
    bool updated = delta_list.erase_earlier_than(get_min_time(), trace_tree);

    if (trace_tree.size() >= m_opt_points)
      // if still too big, remove points based on line simplification
      updated |= delta_list.erase_delta(m_opt_points, trace_tree, no_thin_time);

    if (!updated)
      return false;

  } else if (trace_tree.size() * 2 == m_max_points) {
    // half size, appropriate time to remove old points
    if (!delta_list.erase_earlier_than(get_min_time(), trace_tree))
      return false;

  } else
    return false;

  // must have had change if got this far, so must re-balance tree
  trace_tree.optimize();
  delta_list.update_leaves(trace_tree);
  m_average_delta_distance = delta_list.calc_average_delta_distance(no_thin_time);
  m_average_delta_time = delta_list.calc_average_delta_time(no_thin_time);

  return true;
}

unsigned
Trace::size() const
{
  return trace_tree.size();
}

bool
Trace::empty() const
{
  return delta_list.empty();
}

bool
Trace::is_null(const TracePoint& tp)
{
  return tp.time == null_time;
}

/**
 * Utility class to add tracepoints satisfying a time range to a set
 */
class TracePointSetFilterInserter
{
public:
  /** 
   * Constructor
   * 
   * @param the_set Set to add to
   * @param _min_time Min time (before which to ignore samples), (s)
   */
  TracePointSetFilterInserter(TracePointSet& the_set, 
                              const unsigned _min_time = 0) :
    m_set(&the_set), min_time(_min_time) {}

  /** 
   * Set operator; adds point to set if time satisfied
   *
   * @return Reference to this
   */
  TracePointSetFilterInserter& operator=(const TracePoint& val) {
    if (val.time >= min_time)
      m_set->insert(val);

    return *this;
  }

  /** 
   * Dummy dereference operator
   *
   * @return Reference to this
   */
  TracePointSetFilterInserter& operator*() {
    return *this;
  }

  /** 
   * Dummy increment operator
   * 
   * @param x N to increment
   * 
   * @return Reference to this
   */
  TracePointSetFilterInserter& operator++(const int x) {
    return *this;
  }

private:
  TracePointSet* m_set;
  unsigned min_time;
};

TracePointVector
Trace::find_within_range(const GeoPoint &loc, const fixed range,
                         const unsigned mintime, const fixed resolution) const
{
  if (empty()) 
    return TracePointVector();
  
  TracePoint bb_target(loc);
  bb_target.project(task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);

  // TracePointVector vectors;
  TracePointSet tset;
  TracePointSetFilterInserter filter(tset, mintime);
  trace_tree.find_within_range(bb_target, mrange, filter);

  if (positive(resolution)) {
    const unsigned rrange = task_projection.project_range(loc, resolution);
    TracePointList tlist(tset.begin(), tset.end());
    thin_trace_resolution(tlist, rrange * rrange);
    return TracePointVector(tlist.begin(), tlist.end());
  }

  return TracePointVector(tset.begin(), tset.end());
}

static void
adjust_links(const TracePoint& previous, const TracePoint& obj,
    TracePoint& next)
{
  if (obj.last_time == previous.time &&
      next.last_time == obj.time)
    next.last_time = previous.time;
}

void 
Trace::thin_trace_resolution(TracePointList& tlist, const unsigned mrange_sq)
{
  if (tlist.size() < 2)
    return;

  TracePointList::iterator it_last = tlist.begin();
  TracePointList::iterator it = tlist.begin();
  ++it;
  TracePointList::iterator it_next = it;
  ++it_next;

  for (; it_next != tlist.end();) {
    if (it->approx_sq_dist(*it_last) < mrange_sq) {
      adjust_links(*it_last, *it, *it_next);
      it = tlist.erase(it);
      it_next = it;
      ++it_next;
    } else {
      ++it;
      ++it_next;
      ++it_last;
    }
  }
}

bool
Trace::inside_time_window(unsigned time) const
{
  if (m_last_point.time == null_time)
    return true;

  return time + m_max_time >= m_last_point.time;
}

void
Trace::get_trace_edges(TracePointVector &v) const
{
  v.clear();

  if (trace_tree.size() < 2)
    return;

  // special case - just look for points within time range
  TracePoint p;
  unsigned tmin = null_time;
  TraceTree::const_iterator tend = trace_tree.end();
  for (TraceTree::const_iterator tr = trace_tree.begin();
       tr != tend; ++tr) {
    if ((tr->time< tmin) && inside_time_window(tr->time)) {
      p = *tr;
      tmin = tr->time;
    }
  }
  if ((tmin != null_time) && (m_last_point.time != null_time)) {
    v.reserve(2);
    v.push_back(p);
    v.push_back(m_last_point);
  }
}
