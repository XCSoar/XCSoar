// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/PeriodClock.hpp"

class TaskConstraintsMonitor {
  PeriodClock max_start_speed_clock;

public:
  void Reset() {
    max_start_speed_clock.Reset();
  }

  void Check();
};
