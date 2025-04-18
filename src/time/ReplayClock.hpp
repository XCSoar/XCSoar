// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Stamp.hpp"

/**
 * This class provides a synthetic clock value (for NMEAInfo::clock)
 * fed from a wall-clock time.  This is useful for providing good
 * clock sources for replayed computations.
 *
 * Call Reset() before first use.
 */
class ReplayClock {
  using Duration = FloatDuration;

  TimeStamp clock;

  TimeStamp last_time;

public:
  void Reset() {
    clock = {};
    last_time = TimeStamp::Undefined();
  }

  /**
   * Compute the next clock value.
   *
   * @param time the wallclock time; -1 means unknown
   */
  TimeStamp NextClock(TimeStamp time) noexcept {
    Duration offset;
    if (!time.IsDefined() || !last_time.IsDefined() || time < last_time)
      /* we have no (usable) wall clock time (yet): fully synthesised
         clock; increment by one second on each iteration */
      offset = std::chrono::seconds{1};
    else if (time > last_time)
      /* apply the delta between the two wall clock times to the clock
         value */
      offset = time - last_time;
    else
      /* wall clock time was not updated: apply only a very small
         offset */
      offset = std::chrono::milliseconds{10};

    last_time = time;
    return clock += offset;
  }
};
