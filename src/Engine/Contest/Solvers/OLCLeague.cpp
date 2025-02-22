// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OLCLeague.hpp"
#include "Trace/Trace.hpp"
#include "Cast.hpp"


/**
 * Ruleset: https://www.onlinecontest.org/olc-3.0/segelflugszene/cms.html?url=rules_overview/b5_de
 */
OLCLeague::OLCLeague(const Trace &_trace) noexcept
  :AbstractContest(0), trace(_trace)
{
}

void
OLCLeague::Reset() noexcept
{
  AbstractContest::Reset();
  solution_classic.clear();
  solution.clear();
  for (unsigned i=0; i<5; ++i) {
    solution.append().Clear();
  }
}

SolverResult
OLCLeague::Solve([[maybe_unused]] bool exhaustive) noexcept
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

const ContestTraceVector &
OLCLeague::GetCurrentPath() const noexcept
{
  return solution;
}

ContestResult
OLCLeague::CalculateResult() const noexcept
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
  result.score = ApplyShiftedHandicap(result.distance / 2000);
  return result;
}
