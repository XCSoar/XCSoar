// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeglideFree.hpp"
#include <algorithm>
#include <cmath>

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

  // Distance: 1.0 raw points per km
  double distance_raw = result_distance.distance / 1000.0 * 1.0;
  // FAI: 0.3 raw points per km
  double fai_raw = result_fai.distance / 1000.0 * 0.3;
  // OR: 0.2 raw points per km
  double or_raw = result_or.distance / 1000.0 * 0.2;

  // Maximum of FAI or OR constitutes the area bonus
  double area_bonus = std::max(fai_raw, or_raw);
  // Free raw points = distance + area bonus
  double free_raw = distance_raw + area_bonus;

  // Multiply by 100 and divide by DMSt index
  double score = ApplyHandicap(free_raw);

  // Minimum score is 50 points
  if (score < 50.0)
    score = 0.0;

  // Round to two digits
  result.score = std::round(score * 100.0) / 100.0;

  return result;
}
