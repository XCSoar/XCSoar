// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindowStatsComputer.hpp"
#include "Task/Stats/TaskStats.hpp"

void
WindowStatsComputer::Compute(TimeStamp time, const TaskStats &task_stats,
                             WindowStats &stats) noexcept
{
  if (!time.IsDefined())
    return;

  if (!task_stats.task_valid || !task_stats.start.HasStarted() ||
      !task_stats.total.travelled.IsDefined()) {
    Reset();
    stats.Reset();
    return;
  }

  if (task_stats.task_finished)
    return;

  const auto dt = minute_clock.Update(time, std::chrono::seconds{59},
                                      std::chrono::minutes{3});
  if (dt.count() < 0) {
    Reset();
    stats.Reset();
    return;
  }

  if (dt.count() <= 0)
    return;

  travelled_distance.Push(time.ToDuration().count(),
                          task_stats.total.travelled.GetDistance());

  stats.duration = FloatDuration{travelled_distance.GetDeltaXChecked()};
  if (stats.duration > FloatDuration{}) {
    stats.distance = travelled_distance.GetDeltaY();
    stats.speed = stats.distance / stats.duration.count();
  }
}
