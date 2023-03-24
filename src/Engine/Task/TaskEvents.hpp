// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class TaskWaypoint;
struct Waypoint;

/**
 * Class used to provide feedback based on events that can be triggered
 * by the task system.  Typically this would be specialised by the client
 * to hook up to end-user code.
 */
class TaskEvents
{
public:
  /**
   * Called when the aircraft enters a turnpoint observation zone
   *
   * @param tp The turnpoint entered
   */
  virtual void EnterTransition([[maybe_unused]] const TaskWaypoint &tp) noexcept {}

  /**
   * Called when the aircraft exits a turnpoint observation zone
   *
   * @param tp The turnpoint the aircraft has exited
   */
  virtual void ExitTransition([[maybe_unused]] const TaskWaypoint &tp) noexcept {}

  /**
   * Called when auto-advance has changed the active
   * task point in an ordered task
   *
   * @param tp The turnpoint that is now active after auto-advance
   * @param i The task sequence number after auto-advance
   */
  virtual void ActiveAdvanced([[maybe_unused]] const TaskWaypoint &tp,
                              [[maybe_unused]] const int i) noexcept {}

  /**
   * Called when a task point can be advanced but the advance needs
   * to be armed
   *
   * @param tp The taskpoint waiting to be armed
   */
  virtual void RequestArm([[maybe_unused]] const TaskWaypoint &tp) noexcept {}

  /**
   * Called when orderd task has started
   */
  virtual void TaskStart() noexcept {}

  /**
   * Called when orderd task has finished
   */
  virtual void TaskFinish() noexcept {}
};
