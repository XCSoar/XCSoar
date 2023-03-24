// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/FloatDuration.hxx"

#include <type_traits>

struct ContestResult
{
  /** Score (pts) according to contest rule */
  double score;
  /** Optimum distance (m) travelled according to contest rule */
  double distance;
  /** Time (s) of optimised OLC path */
  FloatDuration time;

  constexpr void Reset() noexcept {
    score = 0;
    distance = 0;
    time = {};
  }

  constexpr bool IsDefined() const noexcept {
    return score > 0;
  }

  /**
   * Returns the average speed on the optimised path [m/s].  Returns
   * zero if the result is invalid.
   */
  constexpr double GetSpeed() const noexcept {
    return time.count() > 0
      ? distance / time.count()
      : 0.;
  }
};

static_assert(std::is_trivial<ContestResult>::value, "type is not trivial");
