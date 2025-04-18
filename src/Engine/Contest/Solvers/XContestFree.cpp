// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XContestFree.hpp"

XContestFree::XContestFree(const Trace &_trace,
                           const bool _is_dhv) noexcept
  :ContestDijkstra(_trace, true, 4, 1000),
   is_dhv(_is_dhv) {}

ContestResult
XContestFree::CalculateResult() const noexcept
{
  ContestResult result = ContestDijkstra::CalculateResult();
  // DHV-XC: 1.5 points per km
  // XContest: 1.0 points per km
  const auto score_factor = is_dhv ? 0.0015 : 0.0010;
  result.score = ApplyHandicap(result.distance * score_factor);
  return result;
}
