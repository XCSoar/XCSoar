// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Task/TaskEvents.hpp"

/**
 * TaskEvents to produce simple text output of events for testing
 */
class TaskEventsPrint:
  public TaskEvents
{
  /** Option to enable basic output on events (for testing) */
  bool verbose;

public:
  TaskEventsPrint(const bool _verbose): 
    TaskEvents(),
    verbose(_verbose) {};

  /* virtual methods from class TaskEvents */
  void EnterTransition(const TaskWaypoint& tp) noexcept override;
  void ExitTransition(const TaskWaypoint &tp) noexcept override;
  void ActiveAdvanced(const TaskWaypoint &tp, const int i) noexcept override;
  void RequestArm(const TaskWaypoint &tp) noexcept override;
  void TaskStart() noexcept override;
  void TaskFinish() noexcept override;
};
