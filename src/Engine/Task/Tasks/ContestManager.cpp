#include "ContestManager.hpp"

#include "Task/TaskStats/CommonStats.hpp"
#include "Trace/Trace.hpp"

#ifdef DO_PRINT
#include <stdio.h>
#endif

ContestManager::ContestManager(const Contests _contest,
                             ContestResult &_result,
                             const Trace& trace_full,
                             const Trace& trace_sprint):
  contest(_contest),
  result(_result),
  trace_full(trace_full),
  trace_sprint(trace_sprint),
  olc_sprint(trace_points_sprint),
  olc_fai(trace_points_full),
  olc_classic(trace_points_full)
{
  reset();
}

bool 
ContestManager::update_trace_sample(const AIRCRAFT_STATE &state,
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
ContestManager::update_sample(const AIRCRAFT_STATE &state)
{
  return update_trace_sample(state, trace_points_full) |
         update_trace_sample(state, trace_points_sprint);
}

#ifdef INSTRUMENT_TASK
extern long count_olc;
#endif

bool
ContestManager::run_contest(AbstractContest &contest)
{
  // run solver, return immediately if further processing is required
  // by subsequent calls
  if (!contest.solve())
    return false;

  // if no improved solution was found, must have finished processing
  // with invalid data so need to retrieve new trace
  if (!contest.score(result)) {
    update_trace();
    return true;
  }

  // solver finished and improved solution was found.  save solution
  // and retrieve new trace.

  contest.copy_solution(solution);
  update_trace();

#ifdef INSTRUMENT_TASK
  count_olc++;
#endif

  return true;
}

void
ContestManager::update_trace()
{
  trace_points_full = trace_full.get_trace_points(300);
  trace_points_sprint = trace_sprint.get_trace_points(300);
}

bool 
ContestManager::update_idle()
{
  // \todo: possibly scan each type in a round robin fashion?
  bool retval = false;

  if (trace_points_full.size() < 10)
    update_trace();

  switch (contest) {
  case OLC_Sprint:
    retval = run_contest(olc_sprint);
    break;
  case OLC_FAI:
    retval = run_contest(olc_fai);
    break;
  case OLC_Classic:
    retval = run_contest(olc_classic);
    break;
  };

  if (retval) {
#ifdef DO_PRINT
    printf("# size %d/%d dist %g\n",
           trace_points_full.size(), 
           trace_points_sprint.size(), 
           (double)result.distance);
#endif
  }

  return retval;
}

void
ContestManager::reset()
{
  trace_points_full.clear();
  trace_points_sprint.clear();
  solution.clear();
  olc_sprint.reset();
  olc_fai.reset();
  olc_classic.reset();
}

const TracePointVector& 
ContestManager::get_contest_solution() const
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
