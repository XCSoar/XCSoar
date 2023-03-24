// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskStatsComputer.hpp"
#include "Task/Stats/TaskStats.hpp"

void
TaskStatsComputer::Reset(TaskStats &data)
{
  total.Reset(data.total);
  current_leg.Reset(data.current_leg);

  data.last_hour.Reset();
  window.Reset();

  inst_speed_slow.Design(180, false);
  inst_speed_fast.Design(15, false);
  inst_speed_slow.Reset(0);
  inst_speed_fast.Reset(0);
}

void
TaskStatsComputer::ComputeWindow(TimeStamp time, TaskStats &data) noexcept
{
  window.Compute(time, data, data.last_hour);
}
