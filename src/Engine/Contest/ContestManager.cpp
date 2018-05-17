/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

ContestManager::ContestManager(const Contest _contest,
                               const Trace &trace_full,
                               const Trace &trace_triangle,
                               const Trace &trace_sprint,
                               bool predict_triangle)
  :contest(_contest),
   olc_sprint(trace_sprint),
   olc_fai(trace_triangle, predict_triangle),
   olc_classic(trace_full),
   olc_league(trace_sprint),
   olc_plus(),
   dmst_quad(trace_full),
   xcontest_free(trace_full, false),
   xcontest_triangle(trace_triangle, predict_triangle, false),
   dhv_xc_free(trace_full, true),
   dhv_xc_triangle(trace_triangle, predict_triangle, true),
   sis_at(trace_full),
   net_coupe(trace_full)
{
  Reset();
}

void
ContestManager::SetIncremental(bool incremental)
{
  olc_sprint.SetIncremental(incremental);
  olc_fai.SetIncremental(incremental);
  olc_classic.SetIncremental(incremental);
  dmst_quad.SetIncremental(incremental);
  xcontest_free.SetIncremental(incremental);
  xcontest_triangle.SetIncremental(incremental);
  dhv_xc_free.SetIncremental(incremental);
  dhv_xc_triangle.SetIncremental(incremental);
  sis_at.SetIncremental(incremental);
  net_coupe.SetIncremental(incremental);
}

void
ContestManager::SetPredicted(const TracePoint &predicted)
{
  if (olc_classic.SetPredicted(predicted)) {
    olc_league.Reset();
    olc_plus.Reset();

    if (contest == Contest::OLC_CLASSIC || contest == Contest::OLC_LEAGUE ||
        contest == Contest::OLC_PLUS)
      stats.Reset();
  }

  if (dmst_quad.SetPredicted(predicted) &&
      contest == Contest::DMST)
    stats.Reset();
}

void
ContestManager::SetHandicap(unsigned handicap)
{
  olc_sprint.SetHandicap(handicap);
  olc_fai.SetHandicap(handicap);
  olc_classic.SetHandicap(handicap);
  olc_league.SetHandicap(handicap);
  olc_plus.SetHandicap(handicap);
  dmst_quad.SetHandicap(handicap);
  xcontest_free.SetHandicap(handicap);
  xcontest_triangle.SetHandicap(handicap);
  dhv_xc_free.SetHandicap(handicap);
  dhv_xc_triangle.SetHandicap(handicap);
  sis_at.SetHandicap(handicap);
  net_coupe.SetHandicap(handicap);
}

static bool
RunContest(AbstractContest &_contest,
           ContestResult &result, ContestTraceVector &solution,
           bool exhaustive)
{
  // run solver, return immediately if further processing is required
  // by subsequent calls
  SolverResult r = _contest.Solve(exhaustive);
  if (r != SolverResult::VALID)
    return false;

  // if no improved solution was found, must have finished processing
  // with invalid data
  result = _contest.GetBestResult();

  // solver finished and improved solution was found.  save solution
  // and retrieve new trace.

  solution = _contest.GetBestSolution();

  return true;
}

bool
ContestManager::UpdateIdle(bool exhaustive)
{
  bool retval = false;

  switch (contest) {
  case Contest::NONE:
    break;

  case Contest::OLC_SPRINT:
    retval = RunContest(olc_sprint, stats.result[0],
                        stats.solution[0], exhaustive);
    break;

  case Contest::OLC_FAI:
    retval = RunContest(olc_fai, stats.result[0],
                        stats.solution[0], exhaustive);
    break;

  case Contest::OLC_CLASSIC:
    retval = RunContest(olc_classic, stats.result[0],
                        stats.solution[0], exhaustive);
    break;

  case Contest::OLC_LEAGUE:
    retval = RunContest(olc_classic, stats.result[1],
                        stats.solution[1], exhaustive);

    olc_league.Feed(stats.solution[1]);

    retval |= RunContest(olc_league, stats.result[0],
                         stats.solution[0], exhaustive);
    break;

  case Contest::OLC_PLUS:
    retval = RunContest(olc_classic, stats.result[0],
                        stats.solution[0], exhaustive);

    retval |= RunContest(olc_fai, stats.result[1],
                         stats.solution[1], exhaustive);

    if (retval) {
      olc_plus.Feed(stats.result[0], stats.solution[0],
                    stats.result[1], stats.solution[1]);

      RunContest(olc_plus, stats.result[2],
                 stats.solution[2], exhaustive);
    }

    break;

  case Contest::DMST:
    retval = RunContest(dmst_quad, stats.result[0],
                        stats.solution[0], exhaustive);
    break;

  case Contest::XCONTEST:
    retval = RunContest(xcontest_free, stats.result[0],
                        stats.solution[0], exhaustive);
    retval |= RunContest(xcontest_triangle, stats.result[1],
                         stats.solution[1], exhaustive);
    break;

  case Contest::DHV_XC:
    retval = RunContest(dhv_xc_free, stats.result[0],
                        stats.solution[0], exhaustive);
    retval |= RunContest(dhv_xc_triangle, stats.result[1],
                         stats.solution[1], exhaustive);
    break;

  case Contest::SIS_AT:
    retval = RunContest(sis_at, stats.result[0],
                        stats.solution[0], exhaustive);
    break;

  case Contest::NET_COUPE:
    retval = RunContest(net_coupe, stats.result[0],
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
  dmst_quad.Reset();
  xcontest_free.Reset();
  xcontest_triangle.Reset();
  dhv_xc_free.Reset();
  dhv_xc_triangle.Reset();
  sis_at.Reset();
  net_coupe.Reset();
}

/*

- SearchPointVector find self intersections (for OLC-FAI)
  -- eliminate bad candidates
  -- remaining candidates are potential finish points

- Possible use of convex reduction for approximate solution to triangle

- Specialised thinning routine; store max/min altitude etc
*/
