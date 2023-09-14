// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskType.hpp"

struct AircraftState;
class TaskWaypoint;
class TaskPoint;
class GlidePolar;

/**
 *  Abstract interface for all tasks and task manager.  This defines
 *  the functionality all tasks should have, and also provides some convenience
 *  methods that will be common to all tasks.
 */
class TaskInterface
{
  const TaskType type;

public:
  constexpr
  TaskInterface(const TaskType _type) noexcept:type(_type) {}

  constexpr
  TaskType GetType() const noexcept {
    return type;
  }

  /**
   * Size of task
   *
   * @return Number of taskpoints in task
   */
  [[gnu::pure]]
  virtual unsigned TaskSize() const noexcept = 0;

  /**
   * Set index in sequence of active task point.  Concrete classes providing
   * this method should ensure the index is valid.
   *
   * @param new_index Desired sequence index of active task point
   */
  virtual void SetActiveTaskPoint(unsigned new_index) noexcept = 0;

  /**
   * Accessor for active task point.  Typically could be used
   * to access information about the task point for user feedback.
   *
   * @return Active task point
   */
  [[gnu::pure]]
  virtual TaskWaypoint* GetActiveTaskPoint() const noexcept = 0;

  /**
   * Determine whether active task point optionally shifted points to
   * a valid task point.
   *
   * @param index_offset offset (default 0)
   */
  [[gnu::pure]]
  virtual bool IsValidTaskPoint(const int index_offset) const noexcept = 0;

  /**
   * Update internal states as flight progresses.  This may perform
   * callbacks to the task_events, and advance the active task point
   * based on task_behaviour.
   *
   * @param state_now Aircraft state at this time step
   * @param state_last Aircraft state at previous time step
   *
   * @return True if internal state changed
   */
  virtual bool Update(const AircraftState &state_now,
                      const AircraftState &state_last,
                      const GlidePolar &glide_polar) noexcept = 0;

  /**
   * Update internal states (non-essential) for housework, or where functions are slow
   * and would cause loss to real-time performance.
   *
   * @param state_now Aircraft state at this time step
   *
   * @return True if internal state changed
   */
  virtual bool UpdateIdle(const AircraftState &state_now,
                          const GlidePolar &glide_polar) noexcept = 0;
};
