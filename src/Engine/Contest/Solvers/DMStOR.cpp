// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DMStOR.hpp"

DMStOR::DMStOR(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 2, 1000) {}

ContestResult
DMStOR::CalculateResult() const noexcept
{
  ContestResult result = ContestDijkstra::CalculateResult();

  // Section 4.1.5.3: +30% out-and-return bonus
  // Formula: distance_km * 1.3 * 100 / index
  double score = ApplyHandicap(result.distance / 1000.0 * 1.3);

  // Section 4: minimum 50 points for distance scoring
  result.score = score >= 50.0 ? score : 0.0;

  return result;
}
