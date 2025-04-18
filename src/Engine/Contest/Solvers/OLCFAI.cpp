// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OLCFAI.hpp"

OLCFAI::OLCFAI(const Trace &_trace, bool predict) noexcept
  :TriangleContest(_trace, predict, 1000)
{
}

ContestResult
OLCFAI::CalculateResult() const noexcept
{
  ContestResult result = TriangleContest::CalculateResult();
  // 0.3 points per km
  result.score = ApplyHandicap(result.distance * 0.0003);
  return result;
}
