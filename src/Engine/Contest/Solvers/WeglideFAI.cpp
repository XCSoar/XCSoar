// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeglideFAI.hpp"

WeglideFAI::WeglideFAI(const Trace &_trace, bool predict) noexcept
  :TriangleContest(_trace, predict, 1000)
{
}

ContestResult
WeglideFAI::CalculateResult() const noexcept
{
  ContestResult result = TriangleContest::CalculateResult();
  // 1 point per km 
  result.score = ApplyHandicap(result.distance * 0.001);

  return result;
}
