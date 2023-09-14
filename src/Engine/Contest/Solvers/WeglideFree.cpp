// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeglideFree.hpp"

WeglideFree::WeglideFree() noexcept
  :AbstractContest(0)
{
}

void
WeglideFree::Reset() noexcept
{
  AbstractContest::Reset();
  solution_distance.clear();
  solution_or.clear();
  solution_fai.clear();
  result_distance.Reset();
  result_or.Reset();
  result_fai.Reset();
}

SolverResult
WeglideFree::Solve([[maybe_unused]] bool exhaustive) noexcept
{
  return SaveSolution()
    ? SolverResult::VALID
    : SolverResult::FAILED;
}

const ContestTraceVector &
WeglideFree::GetCurrentPath() const noexcept
{
  return solution_distance;
}

ContestResult
WeglideFree::CalculateResult() const noexcept
{
  ContestResult result = result_distance;

  auto area_score = 
    (result_or.distance * 0.2 > result_fai.distance * 0.3)
    ? (result_or.distance * 0.2)
    : (result_fai.distance * 0.3);

  result.score = ApplyHandicap((result_distance.distance + area_score) 
                                / 1000);

  return result;
}
