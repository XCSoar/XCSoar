// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DMStQuad.hpp"

DMStQuad::DMStQuad(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 4, 1000) {}

ContestResult
DMStQuad::CalculateResult() const noexcept
{
  ContestResult result = ContestDijkstra::CalculateResult();

  // Section 4.1.4: distance_km * 100 / index (no bonus)
  double score = ApplyHandicap(result.distance / 1000.0);

  // Section 4: minimum 50 points for distance scoring
  result.score = score >= 50.0 ? score : 0.0;

  return result;
}
