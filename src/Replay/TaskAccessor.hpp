// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"

class TaskAccessor final {
  TaskManager &task_manager;
  const double floor_alt;

public:
  TaskAccessor(TaskManager &_task_manager, double _floor_alt) noexcept
    :task_manager(_task_manager), floor_alt(_floor_alt) {}

  [[gnu::pure]]
  bool IsOrdered() const noexcept {
    const TaskInterface *task = task_manager.GetActiveTask();
    return task != nullptr && task->GetType() == TaskType::ORDERED;
  }

  [[gnu::pure]]
  bool IsEmpty() const noexcept {
    const TaskInterface *task = task_manager.GetActiveTask();
    return task == nullptr || task->TaskSize() == 0;
  }

  [[gnu::pure]]
  bool IsFinished() const noexcept {
    return task_manager.GetOrderedTask().GetStats().task_finished;
  }

  [[gnu::pure]]
  bool IsStarted() const noexcept {
    return task_manager.GetOrderedTask().GetStats().start.HasStarted();
  }

  [[gnu::pure]]
  GeoPoint GetRandomOZPoint(unsigned index, const double noise) const noexcept {
    return task_manager.RandomPointInTask(index, noise);
  }

  [[gnu::pure]]
  unsigned size() const noexcept {
    return task_manager.TaskSize();
  }

  [[gnu::pure]]
  GeoPoint GetActiveTaskPointLocation() const noexcept {
    return task_manager.GetActiveTaskPoint()->GetLocation();
  }

  [[gnu::pure]]
  bool HasEntered(unsigned index) const noexcept {
    const TaskInterface *task = task_manager.GetActiveTask();
    if (task == nullptr || task->GetType() != TaskType::ORDERED)
      return true;

    const OrderedTask &o_task = *(const OrderedTask *)task;
    return o_task.IsValidIndex(index) &&
      o_task.GetTaskPoint(index).HasEntered();
  }

  [[gnu::pure]]
  const ElementStat GetLegStats() const noexcept {
    return task_manager.GetStats().current_leg;
  }

  [[gnu::pure]]
  double GetTargetHeight() const noexcept {
    if (task_manager.GetActiveTaskPoint())
      return std::max(floor_alt,
                      task_manager.GetActiveTaskPoint()->GetElevation());
    else
      return floor_alt;
  }

  [[gnu::pure]]
  double GetRemainingAltitudeDifference() const noexcept {
    return task_manager.GetStats().total.solution_remaining.altitude_difference;
  }

  [[gnu::pure]]
  GlidePolar GetGlidePolar() const noexcept {
    return task_manager.GetGlidePolar();
  }

  void SetActiveTaskPoint(unsigned index) noexcept {
    task_manager.SetActiveTaskPoint(index);
  }

  [[gnu::pure]]
  unsigned GetActiveTaskPointIndex() const noexcept {
    return task_manager.GetActiveTaskPointIndex();
  }
};
