// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Stamp.hpp"
#include "util/Compiler.h"

/**
 * Calculates the time difference between two events, and detects time
 * warps.
 */
class DeltaTime {
  /**
   * The time stamp of the previous call.  A negative value means
   * "unavailable".
   */
  TimeStamp last_time;

public:
  void Reset() {
    last_time = TimeStamp::Undefined();
  }

  [[gnu::pure]]
  bool IsDefined() const {
    return last_time.IsDefined();
  }

  /**
   * Update the "last" time stamp, and return the difference.  Returns
   * -1 on time warp.
   *
   * @param current_time the current time stamp or -1 if not known
   * @param min_delta returns zero and does not update the "last" time
   * stamp if the difference is smaller than this value
   * @param warp_tolerance if the time warp is smaller than this
   * value, then zero is returned instead of -1
   * @return the (non-negative) time stamp difference since the last
   * call, or 0 if difference is too small, or -1 on time warp
   */
  FloatDuration Update(TimeStamp current_time,
                       FloatDuration min_delta,
                       FloatDuration warp_tolerance) noexcept;
};
