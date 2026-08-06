// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Task/TaskEvents.hpp"

class GlideComputer;

class GlideComputerTaskEvents final : public TaskEvents {
  GlideComputer* computer;

  bool suppressed = false;

public:
  void SetComputer(GlideComputer &_computer) noexcept;

  /**
   * Stop notifying the pilot, for the duration of a Resume sweep.
   *
   * A sweep replays a whole Flight through the live task engine, so every
   * start, turnpoint advance and finish the pilot already flew is announced
   * again -- a burst of "Task started" and "Next waypoint" the moment the
   * progress dialog closes.
   *
   * Only the notifications are suppressed.  The GlideComputer callbacks these
   * methods also make are pure state, and rebuilding that state is the whole
   * point of the sweep.
   */
  void SetSuppressed(bool _suppressed) noexcept {
    suppressed = _suppressed;
  }

  /* virtual methods from class TaskEvents */
  void EnterTransition(const TaskWaypoint& tp) noexcept override;
  void ActiveAdvanced(const TaskWaypoint &tp, const int i) noexcept override;
  void RequestArm(const TaskWaypoint &tp) noexcept override;
  void TaskStart() noexcept override;
  void TaskFinish() noexcept override;
};
