// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeglideDistance.hpp"
#include "../ContestResult.hpp"

WeglideDistance::WeglideDistance(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 6, 1000) {}

ContestResult
WeglideDistance::CalculateResult() const noexcept
{
  ContestResult result = ContestDijkstra::CalculateResult();
  // 1.0 raw points per km
  result.score = ApplyHandicap(result.distance / 1000.0 * 1.0);
  return result;
}
