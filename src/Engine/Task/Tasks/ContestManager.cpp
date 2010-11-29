#include "ContestManager.hpp"

#include "Task/TaskStats/CommonStats.hpp"
#include "Trace/Trace.hpp"

ContestManager::ContestManager(const Contests _contest,
                               const unsigned &_handicap,
                               ContestResult &_result,
                               const Trace& trace_full,
                               const Trace& trace_sprint):
  contest(_contest),
  result(_result),
  trace_full(trace_full),
  trace_sprint(trace_sprint),
  olc_sprint(trace_sprint, _handicap),
  olc_fai(trace_full, _handicap),
  olc_classic(trace_full, _handicap),
  olc_league(trace_sprint, _handicap),
  olc_plus(trace_full, _handicap)
{
  reset();
}


bool
ContestManager::run_contest(AbstractContest &the_contest, 
                            ContestResult &contest_result,
                            TracePointVector &contest_solution)
{
  // run solver, return immediately if further processing is required
  // by subsequent calls
  if (!the_contest.solve())
    return false;

  // if no improved solution was found, must have finished processing
  // with invalid data
  if (!the_contest.score(contest_result)) {
    return true;
  }

  // solver finished and improved solution was found.  save solution
  // and retrieve new trace.

  the_contest.copy_solution(contest_solution);

  return true;
}


bool 
ContestManager::update_idle()
{
  bool retval = false;
  ContestResult dummy_result;

  switch (contest) {
  case OLC_Sprint:
    retval = run_contest(olc_sprint, result, solution);
    break;
  case OLC_FAI:
    retval = run_contest(olc_fai, result, solution);
    break;
  case OLC_Classic:
    retval = run_contest(olc_classic, result, solution);
    break;
  case OLC_League:
    retval = run_contest(olc_classic, dummy_result, olc_league.get_solution_classic());
    retval |= run_contest(olc_league, result, solution);
    break;
  case OLC_Plus:
    retval = run_contest(olc_classic, olc_plus.get_result_classic(), olc_plus.get_solution_classic());
    retval |= run_contest(olc_fai, olc_plus.get_result_fai(), olc_plus.get_solution_fai());
    if (retval) 
      retval |= run_contest(olc_plus, result, solution);
    break;
  };

  return retval;
}

void
ContestManager::reset()
{
  solution.clear();
  olc_sprint.reset();
  olc_fai.reset();
  olc_classic.reset();
  olc_league.reset();
  olc_plus.reset();
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
