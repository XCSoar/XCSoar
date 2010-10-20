#include "OnlineContest.hpp"

#include "Task/TaskStats/CommonStats.hpp"
#include "Trace/Trace.hpp"

#ifdef DO_PRINT
#include <stdio.h>
#endif

OnlineContest::OnlineContest(const Contests _contest,
                             ContestResult &_result,
                             const Trace& trace_full,
                             const Trace& trace_sprint):
  contest(_contest),
  result(_result),
  trace_full(trace_full),
  trace_sprint(trace_sprint),
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
  }

  return false;
}

bool 
OnlineContest::update_sample(const AIRCRAFT_STATE &state)
{
  return update_trace_sample(state, trace_points_full) |
         update_trace_sample(state, trace_points_sprint);
}

#ifdef INSTRUMENT_TASK
extern long count_olc;
#endif

bool
OnlineContest::run_olc(OLCDijkstra &dijkstra)
{
  if (!dijkstra.solve())
    return false;

  if (!dijkstra.score(result))
    return false;

  dijkstra.copy_solution(solution);
  update_trace();

#ifdef INSTRUMENT_TASK
  count_olc++;
#endif

  return true;
}

void
OnlineContest::update_trace()
{
  trace_points_full = trace_full.get_trace_points(300);
  trace_points_sprint = trace_sprint.get_trace_points(300);
}

bool 
OnlineContest::update_idle()
{
  // \todo: possibly scan each type in a round robin fashion?
  bool retval = false;

  if (trace_points_full.size() < 10)
    update_trace();

  switch (contest) {
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
    printf("# size %d/%d dist %g\n",
           trace_points_full.size(), 
           trace_points_sprint.size(), 
           (double)common_stats.olc.distance);
#endif
  }

  return retval;
}

void
OnlineContest::reset()
{
  trace_points_full.clear();
  trace_points_sprint.clear();
  solution.clear();
  olc_sprint.reset();
  olc_fai.reset();
  olc_classic.reset();
}

const TracePointVector& 
OnlineContest::get_trace_points(bool full_trace) const
{
  return full_trace ? trace_points_full : trace_points_sprint;
}

const TracePointVector& 
OnlineContest::get_olc_points() const
{
  return solution;
}

/*

- SearchPointVector find self intersections (for OLC-FAI)
  -- eliminate bad candidates
  -- remaining candidates are potential finish points

- Possible use of convex reduction for approximate solution to triangle

- Specialised thinning routine; store max/min altitude etc
*/
