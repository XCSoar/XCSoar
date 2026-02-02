// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeglideOR.hpp"
#include "../ContestResult.hpp"

WeglideOR::WeglideOR(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 2, 1000) {}

ContestResult
WeglideOR::CalculateResult() const noexcept
{
  ContestResult result = ContestDijkstra::CalculateResult();
  // 0.2 raw points per km
  result.score = ApplyHandicap(result.distance / 1000.0 * 0.2);
  return result;
}
