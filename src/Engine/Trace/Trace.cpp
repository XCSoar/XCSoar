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
  } else if (state.Time <= m_last_point.time)
    return; // don't add duplicates

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
  m_optimise_time = 0;
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

struct time_out_of_range {
  time_out_of_range(const unsigned the_time = 0):m_time(the_time) {}

  bool operator() (const TracePoint& x) const { return (x.time < m_time); }

  const unsigned m_time;
};


static bool
time_sort(const TracePoint& elem1, 
          const TracePoint& elem2 ) 
{
  return (elem1.time < elem2.time);
}


TracePointVector
Trace::find_within_range(const GEOPOINT &loc, const fixed range,
  const unsigned mintime) const
{
  AIRCRAFT_STATE state; state.Location = loc;
  TracePoint bb_target(state, task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);

  TracePointVector vectors;
  trace_tree.find_within_range(bb_target, mrange, 
                               std::back_inserter(vectors));

  time_out_of_range time_pred(mintime);
  vectors.erase(std::remove_if(vectors.begin(), vectors.end(), time_pred), vectors.end());

  std::sort(vectors.begin(), vectors.end(), time_sort);

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


