// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ElementStatComputer.hpp"
#include "WindowStatsComputer.hpp"
#include "Math/Filter.hpp"

class TaskStats;

class TaskStatsComputer {
public:
  ElementStatComputer total;
  ElementStatComputer current_leg;
  WindowStatsComputer window;

  Filter inst_speed_slow;
  Filter inst_speed_fast;

public:
  /** Reset each element (for incremental speeds). */
  void Reset(TaskStats &data);

  void ComputeWindow(TimeStamp time, TaskStats &data) noexcept;
};
