// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FloatDuration.hxx"
#include "Stamp.hpp"

#include <chrono>

/**
 * Class for GPS-time based time intervals
 */
class GPSClock {
  using Duration = FloatDuration;

  /**
   * A large negative value which ensure that first CheckAdvance()
   * call after Reset() returns true, even if starting XCSoar right
   * after midnight.
   */
  static constexpr TimeStamp RESET_VALUE{FloatDuration{-99999}};

  TimeStamp last = RESET_VALUE;

public:
  /**
   * Initializes the object, setting the last time stamp to "0",
   * i.e. a check() will always succeed.  If you do not want this
   * default behaviour, call update() immediately after creating the
   * object.
   */
  constexpr GPSClock() noexcept = default;

  /**
   * Resets the clock.
   */
  void Reset() {
    last = RESET_VALUE;
  }

  /**
   * Updates the clock.
   */
  void Update(TimeStamp now) noexcept {
    last = now;
  }

  /**
   * Checks whether the GPS time was reversed
   * @param now Current time
   * @return True if time has been reversed, False otherwise
   */
  bool CheckReverse(const TimeStamp now) noexcept {
    if (now<last) {
      Update(now);
      return true;
    } else {
      return false;
    }
  }

  /**
   * Checks whether the specified duration (dt) has passed since the last
   * update. If yes, it updates the time stamp.
   * @param now Current time
   * @param dt The timestep in seconds
   * @return
   */
  bool CheckAdvance(const TimeStamp now,
                    const Duration dt) noexcept {
    if (CheckReverse(now))
      return false;

    if (now >= last + dt) {
      Update(now);
      return true;
    } else
      return false;
  }
};
