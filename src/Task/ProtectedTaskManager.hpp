/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_PROTECTED_TASK_MANAGER_HPP
#define XCSOAR_PROTECTED_TASK_MANAGER_HPP

#include "Thread/Guard.hpp"
#include "Task/TaskManager.hpp"
#include "Engine/Task/Unordered/AbortIntersectionTest.hpp"
#include "Compiler.h"

#include <tchar.h>

class GlidePolar;
class RoutePlannerGlue;
struct RangeAndRadial;

class ReachIntersectionTest: public AbortIntersectionTest {
  const RoutePlannerGlue *route;

public:
  ReachIntersectionTest(): route(NULL) {};

  void SetRoute(const RoutePlannerGlue *_route) {
    route = _route;
  }

  virtual bool Intersects(const AGeoPoint& destination);
};

/**
 * Facade to task/airspace/waypoints as used by threads,
 * to manage locking
 */
class ProtectedTaskManager: public Guard<TaskManager>
{
protected:
  const TaskBehaviour &task_behaviour;
  ReachIntersectionTest intersection_test;

  static const TCHAR default_task_path[];

public:
  ProtectedTaskManager(TaskManager &_task_manager, const TaskBehaviour &tb);

  ~ProtectedTaskManager();

  // common accessors for ui and calc clients
  void SetGlidePolar(const GlidePolar &glide_polar);

  gcc_pure
  const OrderedTaskBehaviour GetOrderedTaskBehaviour() const;

  gcc_pure
  const Waypoint* GetActiveWaypoint() const;

  void IncrementActiveTaskPoint(int offset);
  void IncrementActiveTaskPointArm(int offset);

  bool DoGoto(const Waypoint &wp);

  gcc_pure
  AircraftState GetStartState() const;

  gcc_pure
  fixed GetFinishHeight() const;

  gcc_malloc
  OrderedTask* TaskClone() const;

  /**
   * Copy task into this task
   *
   * @param other OrderedTask to copy
   * @return True if this task changed
   */
  bool TaskCommit(const OrderedTask& that);

  bool TaskSave(const TCHAR *path);

  bool TaskSaveDefault();

  /**
   * Creates an ordered task based on the Default.tsk file
   * Consumer's responsibility to delete task
   *
   * @param waypoints waypoint structure
   * @param failfactory default task type used if Default.tsk is invalid
   * @return OrderedTask from Default.tsk file or if Default.tsk is invalid
   * or non-existent, returns empty task with defaults set by
   * config task defaults
   */
  gcc_malloc
  OrderedTask* TaskCreateDefault(const Waypoints *waypoints,
                                 TaskFactoryType factory);

  bool TaskSave(const TCHAR* path, const OrderedTask& task);

  /** Reset the tasks (as if never flown) */
  void Reset();

  /**
   * Lock/unlock the target from automatic shifts of specified tp
   *
   * @param index index of tp in task
   * @param do_lock Whether to lock the target
   */
  bool TargetLock(const unsigned index, bool do_lock);

  void SetRoutePlanner(const RoutePlannerGlue *_route);

  short GetTerrainBase() const;
};

#endif
