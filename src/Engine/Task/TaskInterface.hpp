/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef TASKINTERFACE_H
#define TASKINTERFACE_H

#include "TaskType.hpp"
#include "Compiler.h"

struct AircraftState;
struct GeoPoint;
struct GeoVector;

struct TaskBehaviour;
class TaskStats;
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
  TaskInterface(const TaskType _type):type(_type) {}

  constexpr
  TaskType GetType() const {
    return type;
  }

  /**
   * Size of task
   *
   * @return Number of taskpoints in task
   */
  gcc_pure
  virtual unsigned TaskSize() const = 0;

  /**
   * Set index in sequence of active task point.  Concrete classes providing
   * this method should ensure the index is valid.
   *
   * @param new_index Desired sequence index of active task point
   */
  virtual void SetActiveTaskPoint(unsigned new_index) = 0;

  /**
   * Accessor for active task point.  Typically could be used
   * to access information about the task point for user feedback.
   *
   * @return Active task point
   */
  gcc_pure
  virtual TaskWaypoint* GetActiveTaskPoint() const = 0;

  /**
   * Determine whether active task point optionally shifted points to
   * a valid task point.
   *
   * @param index_offset offset (default 0)
   */
  gcc_pure
  virtual bool IsValidTaskPoint(const int index_offset) const = 0;

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
                      const GlidePolar &glide_polar) = 0;

  /**
   * Update internal states (non-essential) for housework, or where functions are slow
   * and would cause loss to real-time performance.
   *
   * @param state_now Aircraft state at this time step
   *
   * @return True if internal state changed
   */
  virtual bool UpdateIdle(const AircraftState &state_now,
                          const GlidePolar &glide_polar) = 0;
};

#endif //TASKINTERFACE_H
