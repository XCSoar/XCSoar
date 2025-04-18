// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Task/TaskEvents.hpp"

class GlideComputer;

class GlideComputerTaskEvents final : public TaskEvents {
  GlideComputer* computer;

public:
  void SetComputer(GlideComputer &_computer) noexcept;

  /* virtual methods from class TaskEvents */
  void EnterTransition(const TaskWaypoint& tp) noexcept override;
  void ActiveAdvanced(const TaskWaypoint &tp, const int i) noexcept override;
  void RequestArm(const TaskWaypoint &tp) noexcept override;
  void TaskStart() noexcept override;
  void TaskFinish() noexcept override;
};
