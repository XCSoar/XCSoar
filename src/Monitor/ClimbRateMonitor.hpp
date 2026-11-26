// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/GPSClock.hpp"

/**
 * Speaks the average climb of the last 30 seconds every 30 seconds
 * while circling, so the pilot can keep the eyes outside: "one point
 * five".
 */
class ClimbRateMonitor {
  /** When the climb rate was spoken last, or circling began */
  GPSClock spoken_clock;

public:
  void Reset() noexcept {
    spoken_clock.Reset();
  }

  void Check() noexcept;
};
