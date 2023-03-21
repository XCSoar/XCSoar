// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ContestDijkstra.hpp"

AbstractContest::AbstractContest(const unsigned _finish_alt_diff) noexcept
  :handicap(100),
   finish_alt_diff(_finish_alt_diff)
{
}

void
AbstractContest::Reset() noexcept
{
  best_result.Reset();
}

bool
AbstractContest::UpdateScore() noexcept
{
  // for normal contests, nothing needs to be done
  return false;
}

bool
AbstractContest::SaveSolution() noexcept
{
  ContestResult result = CalculateResult();
  const bool improved = result.score > best_result.score;

  if (!improved)
    return false;

  best_result = result;
  CopySolution(best_solution);
  return true;
}

bool
AbstractContest::IsFinishAltitudeValid(const TracePoint &start,
                                       const TracePoint &finish) const noexcept
{
  return finish.GetIntegerAltitude() + (int)finish_alt_diff >=
    start.GetIntegerAltitude();
}
