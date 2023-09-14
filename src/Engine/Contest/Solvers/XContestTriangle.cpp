// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XContestTriangle.hpp"

XContestTriangle::XContestTriangle(const Trace &_trace,
                                   bool predict, bool _is_dhv) noexcept
  :TriangleContest(_trace, predict),
   is_dhv(_is_dhv) {}

ContestResult
XContestTriangle::CalculateResult() const noexcept
{
  ContestResult result = TriangleContest::CalculateResult();

  if (result.distance > 0) {
    // approximation for now: gap is distance from start to finish
    const auto d_gap = TraceManager::GetPoint(0).GetLocation()
      .Distance(TraceManager::GetPoint(n_points - 1).GetLocation());

    // award no points if gap is >20% of triangle

    if (d_gap > 0.2 * result.distance)
      result.distance = 0;
    else
      result.distance -= d_gap;
  } else
    result.distance = 0;

  // DHV-XC: 2.0 or 1.75 points per km for FAI vs non-FAI triangle
  // XContest: 1.4 or 1.2 points per km for FAI vs non-FAI triangle

  constexpr bool is_fai = true; // TODO: how to set this flag?
  const auto score_factor = is_dhv
    ? (is_fai ? 0.002 : 0.00175)
    : (is_fai ? 0.0014 : 0.0012);

  result.score = ApplyHandicap(result.distance * score_factor);
  return result;
}

SolverResult
XContestTriangle::Solve(bool exhaustive) noexcept
{
  SolverResult result = TriangleContest::Solve(exhaustive);
  if (result != SolverResult::FAILED)
    best_d = 0; // reset heuristic

  return result;
}
