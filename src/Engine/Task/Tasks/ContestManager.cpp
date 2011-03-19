/* Copyright_License {

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
#include "ContestManager.hpp"

#include "Task/TaskStats/CommonStats.hpp"
#include "Trace/Trace.hpp"

ContestManager::ContestManager(const Contests _contest,
                               const unsigned &_handicap,
                               const Trace& trace_full,
                               const Trace& trace_sprint):
  contest(_contest),
  trace_full(trace_full),
  trace_sprint(trace_sprint),
  olc_sprint(trace_sprint, _handicap),
  olc_fai(trace_full, _handicap),
  olc_classic(trace_full, _handicap),
  olc_league(trace_sprint, _handicap),
  olc_plus(trace_full, _handicap),
  olc_xcontest_free(trace_full, _handicap, false),
  olc_xcontest_triangle(trace_full, _handicap, false),
  olc_dhvxc_free(trace_full, _handicap, true),
  olc_dhvxc_triangle(trace_full, _handicap, true),
  olc_sisat(trace_full, _handicap)
{
  reset();
}


bool
ContestManager::run_contest(AbstractContest &the_contest, 
                            ContestResult &contest_result,
                            ContestTraceVector &contest_solution,
                            bool exhaustive)
{
  // run solver, return immediately if further processing is required
  // by subsequent calls
  if (!the_contest.solve(exhaustive))
    return false;

  // if no improved solution was found, must have finished processing
  // with invalid data
  if (!the_contest.score(contest_result))
    return true;

  // solver finished and improved solution was found.  save solution
  // and retrieve new trace.

  the_contest.copy_solution(contest_solution);

  return true;
}

bool 
ContestManager::update_idle(bool exhaustive)
{
  bool retval = false;

  switch (contest) {
  case OLC_Sprint:
    retval = run_contest(olc_sprint, stats.result[0], stats.solution[0], exhaustive);
    break;
  case OLC_FAI:
    retval = run_contest(olc_fai, stats.result[0], stats.solution[0], exhaustive);
    break;
  case OLC_Classic:
    retval = run_contest(olc_classic, stats.result[0], stats.solution[0], exhaustive);
    break;
  case OLC_League:
    retval = run_contest(olc_classic, stats.result[1], stats.solution[1], exhaustive);

    olc_league.get_solution_classic() = stats.solution[1];
    retval |= run_contest(olc_league, stats.result[0], stats.solution[0], exhaustive);
    break;
  case OLC_Plus:
    retval = run_contest(olc_classic, stats.result[0], stats.solution[0], exhaustive);

    olc_plus.get_result_classic() = stats.result[0];
    olc_plus.get_solution_classic() = stats.solution[0];
    retval |= run_contest(olc_fai, stats.result[1], stats.solution[1], exhaustive);

    olc_plus.get_result_fai() = stats.result[1];
    olc_plus.get_solution_fai() = stats.solution[1];
    if (retval) 
      run_contest(olc_plus, stats.result[2], stats.solution[2], exhaustive);

    break;
  case OLC_XContest:
    retval = run_contest(olc_xcontest_free, stats.result[0], stats.solution[0], exhaustive);
    retval |= run_contest(olc_xcontest_triangle, stats.result[1], stats.solution[1], exhaustive);
    break;
  case OLC_DHVXC:
    retval = run_contest(olc_dhvxc_free, stats.result[0], stats.solution[0], exhaustive);
    retval |= run_contest(olc_dhvxc_triangle, stats.result[1], stats.solution[1], exhaustive);
    break;
  case OLC_SISAT:
    retval = run_contest(olc_sisat, stats.result[0], stats.solution[0], exhaustive);
    break;
  };

  return retval;
}

void
ContestManager::reset()
{
  stats.reset();
  olc_sprint.reset();
  olc_fai.reset();
  olc_classic.reset();
  olc_league.reset();
  olc_plus.reset();
  olc_xcontest_free.reset();
  olc_xcontest_triangle.reset();
  olc_dhvxc_free.reset();
  olc_dhvxc_triangle.reset();
  olc_sisat.reset();
}

/*

- SearchPointVector find self intersections (for OLC-FAI)
  -- eliminate bad candidates
  -- remaining candidates are potential finish points

- Possible use of convex reduction for approximate solution to triangle

- Specialised thinning routine; store max/min altitude etc
*/
