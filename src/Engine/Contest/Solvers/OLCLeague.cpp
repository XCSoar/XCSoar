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

#include "OLCLeague.hpp"
#include "Trace/Trace.hpp"
#include "Cast.hpp"

OLCLeague::OLCLeague(const Trace &_trace)
  :AbstractContest(0), trace(_trace)
{
}

void
OLCLeague::Reset()
{
  AbstractContest::Reset();
  solution_classic.clear();
  solution.clear();
  for (unsigned i=0; i<5; ++i) {
    solution.append().Clear();
  }
}

SolverResult
OLCLeague::Solve(bool exhaustive)
{
  if (trace.size() < 2)
    return SolverResult::FAILED;

  const TracePoint &first = trace.front();
  const TracePoint &last = trace.back();

  if (!IsFinishAltitudeValid(first, last))
    return SolverResult::FAILED;

  // solution found, so set start/finish points
  solution[0] = first;
  solution[4] = last;

  // scan through classic solution to find points there to add

  unsigned index_fill = 1;

  for (unsigned index_classic = 1; index_classic + 1 < solution_classic.size();
       ++index_classic) {
    if (solution_classic[index_classic].IsNewerThan(solution[index_fill - 1]) &&
        solution_classic[index_classic].IsOlderThan(last)) {

      solution[index_fill] = solution_classic[index_classic];
      index_fill++;
      if (index_fill == 4)
        break;
    }
  }

  // if insufficient points found, add repeats of previous points

  for (; index_fill < 4; ++index_fill)
    solution[index_fill] = solution[index_fill - 1];

  SaveSolution();
  return SolverResult::VALID;
}

void
OLCLeague::CopySolution(ContestTraceVector &vec) const
{
  vec = solution;
}

ContestResult
OLCLeague::CalculateResult() const
{
  ContestResult result;
  if (!solution[4].IsDefined()) {
    result.Reset();
    return result;
  }

  result.time = solution[4].DeltaTime(solution[0]);
  result.distance = 0;
  for (unsigned i = 0; i < 4; ++i)
    result.distance += solution[i].DistanceTo(solution[i + 1].GetLocation());
  result.score = ApplyShiftedHandicap(result.distance / 2500);
  return result;
}
