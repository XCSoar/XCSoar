// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DMStTriangle.hpp"

DMStTriangle::DMStTriangle(const Trace &_trace, bool predict) noexcept
  :TriangleContest(_trace, predict, 1000)
{
}

ContestResult
DMStTriangle::CalculateResult() const noexcept
{
  ContestResult result = TriangleContest::CalculateResult();

  // Section 4.1.5.1: +40% triangle bonus
  // Formula: distance_km * 1.4 * 100 / index
  double score = ApplyHandicap(result.distance / 1000.0 * 1.4);

  // Section 4: minimum 50 points for distance scoring
  result.score = score >= 50.0 ? score : 0.0;

  return result;
}
