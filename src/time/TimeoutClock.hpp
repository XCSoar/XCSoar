// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PeriodClock.hpp"
#include "util/Compiler.h"

#include <algorithm>

/**
 * A clock that can be used to check whether a timeout has expired.
 */
class TimeoutClock : private PeriodClock {
public:
  template<class Rep, class Period>
  explicit TimeoutClock(const std::chrono::duration<Rep,Period> &max_duration) noexcept {
    UpdateWithOffset(max_duration);
  }

  [[gnu::pure]]
  bool HasExpired() const {
    return Elapsed() > Duration::zero();
  }

  /**
   * Returns the number of milliseconds remaining until the timeout
   * expires.  The time has already expired if the return value is
   * negative.
   */
  [[gnu::pure]]
  std::chrono::steady_clock::duration GetRemainingSigned() const {
    return -Elapsed();
  }

  /**
   * Returns the number of milliseconds remaining until the timeout
   * expires.  The time has already expired if the return value is
   * 0.
   */
  [[gnu::pure]]
  std::chrono::steady_clock::duration GetRemainingOrZero() const {
    return std::max(GetRemainingSigned(),
                    std::chrono::steady_clock::duration::zero());
  }
};
