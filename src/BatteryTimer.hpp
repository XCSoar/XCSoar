// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/PeriodClock.hpp"

class BatteryTimer {
  /* Remind when the internal battery is below 10%, at most every
     5 minutes.  Do not quit: a flight must keep running, and the
     simulator must not exit either. */

  static constexpr unsigned BATTERY_WARNING = 10;
  static constexpr auto BATTERY_REMINDER = std::chrono::minutes(5);

  PeriodClock last_warning;

public:
  void Process();
};
