// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/DifferentialWindowFilter.hpp"
#include "time/DeltaTime.hpp"

struct WindowStats;
class TaskStats;

class WindowStatsComputer {
  DifferentialWindowFilter<60> travelled_distance;

  DeltaTime minute_clock;

public:
  void Reset() {
    travelled_distance.Clear();
    minute_clock.Reset();
  }

  void Compute(TimeStamp time, const TaskStats &task_stats,
               WindowStats &stats) noexcept;
};
