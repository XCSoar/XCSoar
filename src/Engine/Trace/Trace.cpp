#include "Trace.hpp"
#include "Navigation/Aircraft.hpp"
#include <algorithm>

Trace::Trace() :
  m_optimise_time(0)
{
}

void
Trace::append(const AIRCRAFT_STATE& state)
{
  if (empty()) {
    task_projection.reset(state.get_location());
    task_projection.update_fast();
    m_last_point.time = -1;
  } else if ((trace_tree.size() > 0) && (state.Time < m_last_point.time)) {
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
  distance_delta_map[tp.time] = 0-1;
  time_delta_map[tp.time] = 0-1;

  TraceTree::const_iterator it_prev = find_prev(tp);
  if (it_prev != end()) {
    update_delta(find_prev(*it_prev), it_prev, it_this);
  }
}

void 
Trace::update_delta(TraceTree::const_iterator it_prev,
                    TraceTree::const_iterator it,
                    TraceTree::const_iterator it_next)
{
  if ((it == end())
      || (it_prev == end())
      || (it_next == end()))
    return;

  const unsigned d_this = it_prev->approx_dist(*it)+it->approx_dist(*it_next);
  const unsigned d_rem = it_prev->approx_dist(*it_next);
  const unsigned delta = d_this-d_rem;
  distance_delta_map[it->time] = delta;
  time_delta_map[it->time] = std::max(it->dt(), it_next->dt());
}

bool 
Trace::recent(unsigned time) const
{
  return (time+300 >= m_last_point.time);
}

unsigned
Trace::lowest_delta() const
{
  // note this won't calculate delta for recent

  unsigned lowest = 0-1;

  TraceDeltaMap::const_iterator it = distance_delta_map.begin();

  for (++it; it != distance_delta_map.end(); ++it) {
    if (recent(it->first)) {
      return lowest;
    }
    if (it->second < lowest) {
      lowest = it->second;
    }
  }
  return lowest;
}

void
Trace::trim_point()
{
  // note this won't trim if recent

  // if several points exist with equal deltas, this will remove the one
  // with the smallest time step

  unsigned delta = lowest_delta();
  unsigned lowest_dt = 0-1;
  TraceTree::const_iterator candidate = trace_tree.end();

  for (TraceTree::const_iterator it = trace_tree.begin();
       it != trace_tree.end(); ++it) {

    if (!recent(it->time) && (distance_delta_map[it->time] == delta)) {
      const unsigned dt = time_delta_map[it->time];
      if (dt < lowest_dt) {
        lowest_dt = dt;
        candidate = it;
      }
    }
  }
  if (candidate != trace_tree.end()) {
    erase(candidate);
  }
}

void
Trace::optimise()
{
  while (trace_tree.size()> 1000) {
    trim_point();
  }

  trace_tree.optimize();
  m_optimise_time = m_last_point.time;
}

Trace::TraceTree::const_iterator
Trace::find_prev(const TracePoint& tp) const
{
  for (TraceTree::const_iterator it = trace_tree.begin();
       it != trace_tree.end(); ++it) {
    if (it->time == tp.last_time) {
      return it;
    }
  }
  return end();
}

Trace::TraceTree::const_iterator
Trace::find_next(const TracePoint& tp) const
{
  for (TraceTree::const_iterator it = trace_tree.begin();
       it != trace_tree.end(); ++it) {
    if (it->last_time == tp.time) {
      return it;
    }
  }
  return end();
}

void
Trace::erase(TraceTree::const_iterator& rit)
{
  /// @todo merge data for erased point?

  TraceTree::const_iterator it_prev = find_prev(*rit);
  TraceTree::const_iterator it_next = find_next(*rit);

  if ((it_prev == trace_tree.end()) || 
      (it_next == trace_tree.end())) {
    return;
  }

  TracePoint tp_next = *it_next;
  tp_next.last_time = it_prev->time;

  trace_tree.erase(rit);
  trace_tree.erase(it_next);

  distance_delta_map.erase(rit->time);
  time_delta_map.erase(rit->time);

  it_next = trace_tree.insert(tp_next);

  update_delta(find_prev(*it_prev), it_prev, it_next);
  update_delta(it_prev, it_next, find_next(*it_next));
}


void
Trace::clear()
{
  trace_tree.clear();
  trace_tree.optimize();
  m_optimise_time = 0;
  m_last_point.time = -1;

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

class TracePointSetFilterInserter
{
public:
  TracePointSetFilterInserter(TracePointSet& the_set, const unsigned _min_time = 0) :
    m_set(&the_set), min_time(_min_time)
  {
  }

  TracePointSetFilterInserter& operator=(const TracePoint& val) {
    if (val.time>=min_time)
      m_set->insert(val);

    return *this;
  }

  TracePointSetFilterInserter& operator*() {
    return *this;
  }

  TracePointSetFilterInserter& operator++(const int x) {
    return *this;
  }

private:
  TracePointSet* m_set;
  unsigned min_time;
};

TracePointVector
Trace::find_within_range(const GEOPOINT &loc, const fixed range,
                         const unsigned mintime, const fixed resolution) const
{
  AIRCRAFT_STATE state;
  state.Location = loc; state.Time = mintime;
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
Trace::thin_trace(TracePointList& tlist, const unsigned mrange_sq) const
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
