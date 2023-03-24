// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetCoupe.hpp"

NetCoupe::NetCoupe(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 4, 1000) {}

ContestResult
NetCoupe::CalculateResult() const noexcept
{
  ContestResult result = ContestDijkstra::CalculateResult();
  // 0.8 factor for free distance and 1/1000 m -> km
  result.score = ApplyHandicap(result.distance * 0.0008);
  return result;
}

