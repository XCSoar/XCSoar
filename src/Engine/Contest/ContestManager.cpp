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
                               const Trace &trace_full,
                               const Trace &trace_sprint):
  contest(_contest),
  trace_full(trace_full),
  trace_sprint(trace_sprint),
  olc_sprint(trace_sprint),
  olc_fai(trace_full),
  olc_classic(trace_full),
  olc_league(trace_sprint),
  olc_plus(trace_full),
  olc_xcontest_free(trace_full, false),
  olc_xcontest_triangle(trace_full, false),
  olc_dhvxc_free(trace_full, true),
  olc_dhvxc_triangle(trace_full, true),
  olc_sisat(trace_full)
{
  Reset();
}

void
ContestManager::SetHandicap(unsigned handicap)
{
  olc_sprint.SetHandicap(handicap);
  olc_fai.SetHandicap(handicap);
  olc_classic.SetHandicap(handicap);
  olc_league.SetHandicap(handicap);
  olc_plus.SetHandicap(handicap);
  olc_xcontest_free.SetHandicap(handicap);
  olc_xcontest_triangle.SetHandicap(handicap);
  olc_dhvxc_free.SetHandicap(handicap);
  olc_dhvxc_triangle.SetHandicap(handicap);
  olc_sisat.SetHandicap(handicap);
}

bool
ContestManager::RunContest(AbstractContest &_contest,
                           ContestResult &result,
                           ContestTraceVector &solution,
                           bool exhaustive)
{
  // run solver, return immediately if further processing is required
  // by subsequent calls
  if (!_contest.Solve(exhaustive))
    return false;

  // if no improved solution was found, must have finished processing
  // with invalid data
  if (!_contest.Score(result))
    return true;

  // solver finished and improved solution was found.  save solution
  // and retrieve new trace.

  _contest.CopySolution(solution);

  return true;
}

bool 
ContestManager::UpdateIdle(bool exhaustive)
{
  bool retval = false;

  switch (contest) {
  case OLC_Sprint:
    retval = RunContest(olc_sprint, stats.result[0],
                         stats.solution[0], exhaustive);
    break;

  case OLC_FAI:
    retval = RunContest(olc_fai, stats.result[0],
                         stats.solution[0], exhaustive);
    break;

  case OLC_Classic:
    retval = RunContest(olc_classic, stats.result[0],
                         stats.solution[0], exhaustive);
    break;

  case OLC_League:
    retval = RunContest(olc_classic, stats.result[1],
                         stats.solution[1], exhaustive);

    olc_league.get_solution_classic() = stats.solution[1];

    retval |= RunContest(olc_league, stats.result[0],
                          stats.solution[0], exhaustive);
    break;

  case OLC_Plus:
    retval = RunContest(olc_classic, stats.result[0],
                         stats.solution[0], exhaustive);

    olc_plus.get_result_classic() = stats.result[0];
    olc_plus.get_solution_classic() = stats.solution[0];

    retval |= RunContest(olc_fai, stats.result[1],
                          stats.solution[1], exhaustive);

    olc_plus.get_result_fai() = stats.result[1];
    olc_plus.get_solution_fai() = stats.solution[1];

    if (retval) 
      RunContest(olc_plus, stats.result[2],
                  stats.solution[2], exhaustive);

    break;

  case OLC_XContest:
    retval = RunContest(olc_xcontest_free, stats.result[0],
                         stats.solution[0], exhaustive);
    retval |= RunContest(olc_xcontest_triangle, stats.result[1],
                          stats.solution[1], exhaustive);
    break;

  case OLC_DHVXC:
    retval = RunContest(olc_dhvxc_free, stats.result[0],
                         stats.solution[0], exhaustive);
    retval |= RunContest(olc_dhvxc_triangle, stats.result[1],
                          stats.solution[1], exhaustive);
    break;

  case OLC_SISAT:
    retval = RunContest(olc_sisat, stats.result[0],
                         stats.solution[0], exhaustive);
    break;
  };

  return retval;
}

void
ContestManager::Reset()
{
  stats.Reset();
  olc_sprint.Reset();
  olc_fai.Reset();
  olc_classic.Reset();
  olc_league.Reset();
  olc_plus.Reset();
  olc_xcontest_free.Reset();
  olc_xcontest_triangle.Reset();
  olc_dhvxc_free.Reset();
  olc_dhvxc_triangle.Reset();
  olc_sisat.Reset();
}

/*

- SearchPointVector find self intersections (for OLC-FAI)
  -- eliminate bad candidates
  -- remaining candidates are potential finish points

- Possible use of convex reduction for approximate solution to triangle

- Specialised thinning routine; store max/min altitude etc
*/
