// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/FloatDuration.hxx"

struct WindowStats {
  /**
   * The duration of this window [seconds].  A negative value means
   * this object is undefined.
   */
  FloatDuration duration;

  /**
   * The distance travelled in this window.
   */
  double distance;

  /**
   * The quotient of distance and duration.
   */
  double speed;

  constexpr bool IsDefined() const noexcept {
    return duration >= FloatDuration{};
  }

  constexpr void Reset() noexcept {
    duration = FloatDuration{-1};
  }
};
