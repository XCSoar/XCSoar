// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetCoupe.hpp"

NetCoupe::NetCoupe(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 4, 1000) {}

ContestResult
NetCoupe::CalculateResult() const noexcept
{
  ContestResult result = ContestDijkstra::CalculateResult();
  /**
   * FFVP Coupe Fédérale (WeGlide):
   * https://docs.weglide.org/contests/national/ffvp_coupe_federale.html
   *
   * Points = Distance_km x (100 / Handicap) x Success_Index
   *
   * "Free" flight uses Success_Index = 1.0.  The in-flight optimiser does
   * not know an electronically declared task, so we do not apply 1.2 here.
   */
  result.score = ApplyHandicap(result.distance / 1000.0);
  return result;
}

