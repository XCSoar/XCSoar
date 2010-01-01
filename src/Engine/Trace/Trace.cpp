#include "Trace.hpp"
#include "Navigation/Aircraft.hpp"

Trace::Trace():m_optimise_time(0)
{

}

void 
Trace::append(const AIRCRAFT_STATE& state)
{
  if (empty()) {
    task_projection.reset(state.get_location());
    task_projection.update_fast();
    m_last_point.time = 0-1;
  } else if ((trace_tree.size()>0) && (state.Time < m_last_point.time)) {
    clear();
    return;
  }

  TracePoint tp(state, task_projection);
  tp.last_time = m_last_point.time;
  trace_tree.insert(tp);
  m_last_point = tp;
}

void
Trace::optimise()
{
  trace_tree.optimize();
  m_optimise_time = m_last_point.time;
}

void
Trace::clear()
{
  trace_tree.clear();
  trace_tree.optimize();
  m_optimise_time = 0;
  m_last_point.time = 0-1;
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


TracePointVector
Trace::find_within_range(const GEOPOINT &loc, const fixed range,
                         const unsigned mintime, const fixed resolution) const
{
  AIRCRAFT_STATE state; state.Location = loc; state.Time = mintime;
  TracePoint bb_target(state, task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);
  const unsigned rrange = task_projection.project_range(loc, resolution);

//  TracePointVector vectors;
  TracePointSet tset;
  trace_tree.find_within_range(bb_target, mrange, 
                               std::inserter(tset, tset.begin()));

  if (mintime>0) {
    TracePointSet::iterator tit = tset.lower_bound(bb_target);
    tset.erase(tset.begin(), tit);
  }
  if (positive(resolution)) {
    thin_trace(tset, rrange*rrange);
  }
  TracePointVector vectors(tset.begin(), tset.end());
  return vectors;
}


bool 
Trace::optimise_if_old()
{
  if (m_last_point.time > m_optimise_time+60) {    
    optimise();
    return true;
  } else {
    return false;
  }
}


static void adjust_links(const TracePoint& previous, const TracePoint& obj, TracePoint& next)
{
  if ((obj.last_time == previous.time) && (next.last_time == obj.time)) {
    next.last_time = previous.time;
  }
}


void 
Trace::thin_trace(TracePointSet& tset, const unsigned mrange_sq) const
{
/*
  if (tset.size()<2) return;

  TracePointSet::iterator it = tset.begin(); it++;

  it->last_time--;

  for (; it+1 != tset.end(); ) {

    TracePointSet::iterator it_previous = it;
    TracePointSet::iterator it_next = it;

    if (it->approx_sq_dist(*it_previous)<mrange_sq) {
      adjust_links(*it_previous, *it, *it_next);
      tset.erase(it);
    } else {
      ++it;
    }
  }
*/
}


TracePointVector 
Trace::get_trace_points(const unsigned max_points) const
{
  TracePointSet tset;

  for (TraceTree::const_iterator it = begin();
       it != end(); ++it) {
    tset.insert(*it);
  }

  if (!tset.empty()) {

    unsigned mrange = 3;
    do {
      thin_trace(tset, mrange);
      mrange = (mrange*4)/3;
    } while (tset.size()>max_points);
  }

  TracePointVector vectors(tset.begin(), tset.end());
  return vectors;
}
