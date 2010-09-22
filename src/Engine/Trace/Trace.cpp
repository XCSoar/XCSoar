#include "Trace.hpp"
#include "Navigation/Aircraft.hpp"
#include <algorithm>

const unsigned Trace::null_delta = 0 - 1;
const unsigned Trace::null_time = 0 - 1;

Trace::Trace(const unsigned max_time,
             const unsigned recent_time,
             const unsigned max_points) :
  m_optimise_time(null_time),
  m_recent_time(recent_time),
  m_max_time(max_time),
  m_max_points(max_points)
{
}

void
Trace::append(const AIRCRAFT_STATE& state)
{
  if (empty()) {
    task_projection.reset(state.get_location());
    task_projection.update_fast();
    m_last_point.time = null_time;
  } else if (trace_tree.size() > 0 && state.Time < fixed(m_last_point.time)) {
    clear();
    return;
  }

  TracePoint tp(state, task_projection);
  if ((tp.time - m_last_point.time) < 2)
    return;

  tp.last_time = m_last_point.time;
  TraceTree::const_iterator it_this = trace_tree.insert(tp);
  m_last_point = tp;

  // update deltas.  Last point is always high delta
  distance_delta_map[tp.time] = null_delta;
  time_delta_map[tp.time] = null_time;

  TraceTree::const_iterator it_prev = find_prev(tp);
  if (it_prev != end())
    update_delta(find_prev(*it_prev), it_prev, it_this);
}

void 
Trace::update_delta(TraceTree::const_iterator it_prev,
                    TraceTree::const_iterator it,
                    TraceTree::const_iterator it_next)
{
  if ((it == end()) || (it_prev == end()) || (it_next == end()))
    return;

  const unsigned d_this = it_prev->approx_dist(*it) + it->approx_dist(*it_next);
  const unsigned d_rem = it_prev->approx_dist(*it_next);
  const unsigned delta = d_this - d_rem;
  distance_delta_map[it->time] = delta;
  time_delta_map[it->time] = std::max(it->dt(), it_next->dt());
}

bool
Trace::inside_recent_time(unsigned time) const
{
  if (m_last_point.time == null_time)
    return true;

  return (time + m_recent_time >= m_last_point.time);
}

bool
Trace::inside_time_window(unsigned time) const
{
  if (m_last_point.time == null_time)
    return true;

  return inside_recent_time(time) || (time + m_max_time >= m_last_point.time);
}

unsigned
Trace::lowest_delta() const
{
  // note this won't calculate delta for recent

  unsigned lowest = null_delta;

  TraceDeltaMap::const_iterator it = distance_delta_map.begin();
  for (++it; it != distance_delta_map.end(); ++it) {
    if (inside_recent_time(it->first))
      return lowest;

    if (it->second < lowest)
      lowest = it->second;
  }
  return lowest;
}

void 
Trace::erase_earlier_than(unsigned min_time)
{
  // note: this is slow but currently kdtree++ doesn't return a valid
  // iterator after erase() so there's no easier way

  bool found = false;
  do {
    found = false;
    for (TraceTree::const_iterator tr = trace_tree.begin();
         tr != trace_tree.end(); ++tr) {
      if (tr->time < min_time) {
        trace_tree.erase(tr);
        found = true;
        break;
      }
    }
  } while (found);

  distance_delta_map.erase(distance_delta_map.begin(), 
                           distance_delta_map.lower_bound(min_time));

  time_delta_map.erase(time_delta_map.begin(), 
                       time_delta_map.lower_bound(min_time));
}

void
Trace::trim_point_time()
{
  for (TraceDeltaMap::const_iterator it = distance_delta_map.begin(); 
       it != distance_delta_map.end(); ++it) {
    const unsigned this_time = it->first;

    if (inside_time_window(this_time)) {
      erase_earlier_than(this_time);
      distance_delta_map[this_time] = null_delta;
      time_delta_map[this_time] = null_time;

      return;
    }
  }
}

void
Trace::trim_point_delta()
{
  // note this won't trim if recent

  // if several points exist with equal deltas, this will remove the one
  // with the smallest time step

  unsigned delta = lowest_delta();
  unsigned lowest_dt = null_time;
  TraceTree::const_iterator candidate = trace_tree.end();

#ifndef NDEBUG
  const unsigned min_time = distance_delta_map.begin()->first;
#endif

  for (TraceTree::const_iterator it = trace_tree.begin();
       it != trace_tree.end(); ++it) {
    const unsigned this_time = it->time;
    assert(this_time >= min_time);

    if (!inside_recent_time(this_time) &&
        distance_delta_map[this_time] == delta) {
      const unsigned dt = time_delta_map[this_time];
      if (dt < lowest_dt) {
        lowest_dt = dt;
        candidate = it;
      }
    }
  }

  if (candidate != trace_tree.end())
    erase(candidate);
}

void
Trace::optimise()
{
  // first remove points outside max time range
  if (m_max_time != null_time)
    trim_point_time();

  // if still too big, remove points based on line simplification
  while (trace_tree.size()> m_max_points)
    trim_point_delta();

  // re-balance tree 
  trace_tree.optimize();
  m_optimise_time = m_last_point.time;
}

Trace::TraceTree::const_iterator
Trace::find_prev(const TracePoint& tp) const
{
  for (TraceTree::const_iterator it = trace_tree.begin();
       it != trace_tree.end(); ++it)
    if (it->time == tp.last_time)
      return it;

  return end();
}

Trace::TraceTree::const_iterator
Trace::find_next(const TracePoint& tp) const
{
  for (TraceTree::const_iterator it = trace_tree.begin();
       it != trace_tree.end(); ++it)
    if (it->last_time == tp.time)
      return it;

  return end();
}

void
Trace::erase(TraceTree::const_iterator& rit)
{
  /// @todo merge data for erased point?
  if (rit == trace_tree.end())
    return;

  TraceTree::const_iterator it_prev = find_prev(*rit);
  TraceTree::const_iterator it_next = find_next(*rit);

  if ((it_prev == trace_tree.end()) || (it_next == trace_tree.end()))
    return;

  TracePoint tp_next = *it_next;
  tp_next.last_time = it_prev->time;

  distance_delta_map.erase(rit->time);
  time_delta_map.erase(rit->time);

  trace_tree.erase(rit);
  trace_tree.erase(it_next);

  it_next = trace_tree.insert(tp_next);

  update_delta(find_prev(*it_prev), it_prev, it_next);
  update_delta(it_prev, it_next, find_next(*it_next));
}

void
Trace::clear()
{
  trace_tree.clear();
  trace_tree.optimize();
  m_optimise_time = null_time;
  m_last_point.time = null_time;

  distance_delta_map.clear();
  time_delta_map.clear();
}

unsigned
Trace::size() const
{
  return trace_tree.size();
}

bool
Trace::empty() const
{
  return trace_tree.empty();
}

Trace::TraceTree::const_iterator
Trace::begin() const
{
  return trace_tree.begin();
}

Trace::TraceTree::const_iterator
Trace::end() const
{
  return trace_tree.end();
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
    if (val.time>=min_time)
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
  AIRCRAFT_STATE state;
  state.Location = loc;
  state.Time = fixed(mintime);
  TracePoint bb_target(state, task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);
  const unsigned rrange = task_projection.project_range(loc, resolution);

  // TracePointVector vectors;
  TracePointSet tset;
  TracePointSetFilterInserter filter(tset, mintime);
  trace_tree.find_within_range(bb_target, mrange, filter);

  /*
  // std::inserter(tset, tset.begin()));
  if (mintime>0) {
    TracePointSet::iterator tit = tset.lower_bound(bb_target);
    tset.erase(tset.begin(), tit);
  }
  */

  if (positive(resolution)) {
    TracePointList tlist(tset.begin(), tset.end());
    thin_trace(tlist, rrange * rrange);
    return TracePointVector(tlist.begin(), tlist.end());
  } else {
    return TracePointVector(tset.begin(), tset.end());
  }
}

bool
Trace::optimise_if_old()
{
  if (m_last_point.time > m_optimise_time + 60) {
    optimise();
    return true;
  }

  return false;
}

static void
adjust_links(const TracePoint& previous, const TracePoint& obj,
    TracePoint& next)
{
  if ((obj.last_time == previous.time) && (next.last_time == obj.time))
    next.last_time = previous.time;
}

void 
Trace::thin_trace(TracePointList& tlist, const unsigned mrange_sq)
{
  if (tlist.size() < 2)
    return;

  TracePointList::iterator it_prev = tlist.begin();
  TracePointList::iterator it = tlist.begin();
  ++it;
  TracePointList::iterator it_next = it;
  ++it_next;

  for (; it_next != tlist.end();) {
    if (it->approx_sq_dist(*it_prev) < mrange_sq) {
      adjust_links(*it_prev, *it, *it_next);
      it = tlist.erase(it);
      it_next = it;
      ++it_next;
    } else {
      ++it;
      ++it_next;
      ++it_prev;
    }
  }
}

TracePointVector
Trace::get_trace_points(const unsigned max_points) const
{
  TracePointSet tset(begin(), end());

  if (tset.empty())
    return TracePointVector();

  TracePointList tlist(tset.begin(), tset.end());

  unsigned mrange = 3;

  while (tlist.size() > max_points) {
    thin_trace(tlist, mrange);
    mrange = (mrange * 4) / 3;
  }

  return TracePointVector(tlist.begin(), tlist.end());
}
