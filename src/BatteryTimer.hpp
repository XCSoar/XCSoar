// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/PeriodClock.hpp"

class BatteryTimer {
  // Battery status for SIMULATOR mode
  // 10% reminder, 5% exit, 5 minute reminders on warnings

  static constexpr unsigned BATTERY_WARNING = 10;
  static constexpr unsigned BATTERY_EXIT = 5;
  static constexpr auto BATTERY_REMINDER = std::chrono::minutes(5);

  PeriodClock last_warning;

public:
  void Process();
};
