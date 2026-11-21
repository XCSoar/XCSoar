// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/PeriodClock.hpp"

/**
 * Speaks stall, overspeed, unlocked airbrakes, and landing-gear
 * warnings while they apply in flight, and shows them as HUD labels.
 */
class AircraftWarningMonitor {
  PeriodClock stall_clock;
  PeriodClock overspeed_clock;
  PeriodClock airbrake_clock;
  PeriodClock gear_clock;

public:
  void Reset() noexcept {
    stall_clock.Reset();
    overspeed_clock.Reset();
    airbrake_clock.Reset();
    gear_clock.Reset();
  }

  void Check() noexcept;
};
