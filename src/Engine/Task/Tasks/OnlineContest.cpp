#include "OnlineContest.hpp"
#include "Task/TaskEvents.hpp"
#include "Task/TaskBehaviour.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/TaskStats/CommonStats.hpp"
#include "Trace/Trace.hpp"

#ifdef DO_PRINT
#include <stdio.h>
#endif

OnlineContest::OnlineContest(const TaskEvents &te, 
                             const TaskBehaviour &tb,
                             const GlidePolar &gp,
                             CommonStats& stats,
                             const Trace& trace_full,
                             const Trace& trace_sprint):
  m_task_events(te),
  m_task_behaviour(tb),
  m_glide_polar(gp),
  common_stats(stats),
  m_trace_full(trace_full),
  m_trace_sprint(trace_sprint),
  olc_sprint(*this),
  olc_fai(*this),
  olc_classic(*this)
{
  reset();
}


bool 
OnlineContest::update_trace_sample(const AIRCRAFT_STATE &state,
                                   TracePointVector& vec)
{
  if (vec.empty())
    return false;
    
  if (state.NavAltitude < vec.back().NavAltitude) {
    // replace if lower even if not significant distance away
    vec.back().NavAltitude = state.NavAltitude;
    return true;
  } else {
    return false;
  }
}


bool 
OnlineContest::update_sample(const AIRCRAFT_STATE &state)
{
  bool retval = false;
  retval |= update_trace_sample(state, m_trace_points_full);
  retval |= update_trace_sample(state, m_trace_points_sprint);
  return retval;
}


bool
OnlineContest::run_olc(OLCDijkstra &dijkstra)
{
  if (dijkstra.solve()) {
    const fixed score = dijkstra.score(common_stats.distance_olc);
    if (positive(score)) {
      common_stats.time_olc = dijkstra.calc_time();
      if (positive(common_stats.time_olc)) {
        common_stats.speed_olc = dijkstra.calc_distance()/common_stats.time_olc;
      } else {
        common_stats.speed_olc = fixed_zero;
      }
    }
    dijkstra.copy_solution(m_solution);
    update_trace();

    return true;
  } else {
    return false;
  }
}


void
OnlineContest::update_trace()
{
  m_trace_points_full = m_trace_full.get_trace_points(300);
  m_trace_points_sprint = m_trace_sprint.get_trace_points(300);
}


bool 
OnlineContest::update_idle(const AIRCRAFT_STATE &state)
{
  // \todo: possibly scan each type in a round robin fashion?
  bool retval = false;

  if (m_trace_points_full.size()<10) {
    update_trace();
  }

  switch (m_task_behaviour.olc_rules) {
  case OLC_Sprint:
    retval = run_olc(olc_sprint);
    break;
  case OLC_FAI:
    retval = run_olc(olc_fai);
    break;
  case OLC_Classic:
    retval = run_olc(olc_classic);
    break;
  };

  if (retval) {
#ifdef DO_PRINT
    printf("time %d size %d/%d dist %g\n",
           (int)state.Time,
           m_trace_points_full.size(), 
           m_trace_points_sprint.size(), 
           (double)common_stats.distance_olc);
    print();
#endif
  }

  return retval;
}


void
OnlineContest::reset()
{
  m_trace_points_full.clear();
  m_trace_points_sprint.clear();
  m_solution.clear();
  olc_sprint.reset();
  olc_fai.reset();
  olc_classic.reset();
}


const TracePointVector& 
OnlineContest::get_trace_points(bool full_trace) const
{
  return full_trace? m_trace_points_full: m_trace_points_sprint;
}


const TracePointVector& 
OnlineContest::get_olc_points() const
{
  return m_solution;
}


void 
OnlineContest::Accept(TaskPointVisitor& visitor, 
                      const bool reverse) const
{
  /// \todo - visit "OLCPoint"
}


/*

- SearchPointVector find self intersections (for OLC-FAI)
  -- eliminate bad candidates
  -- remaining candidates are potential finish points

- Possible use of convex reduction for approximate solution to triangle

- Specialised thinning routine; store max/min altitude etc
*/
