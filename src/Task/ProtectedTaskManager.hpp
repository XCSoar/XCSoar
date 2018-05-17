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

#ifndef XCSOAR_PROTECTED_TASK_MANAGER_HPP
#define XCSOAR_PROTECTED_TASK_MANAGER_HPP

#include "Thread/Guard.hpp"
#include "Engine/Task/Unordered/AbortIntersectionTest.hpp"
#include "Engine/Waypoint/Ptr.hpp"
#include "Compiler.h"

struct AGeoPoint;
struct TaskBehaviour;
struct OrderedTaskSettings;
class Path;
class GlidePolar;
class RoutePlannerGlue;
class OrderedTask;
class TaskManager;

class ReachIntersectionTest: public AbortIntersectionTest {
  const RoutePlannerGlue *route;

public:
  ReachIntersectionTest():route(nullptr) {};

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

public:
  ProtectedTaskManager(TaskManager &_task_manager, const TaskBehaviour &tb);

  ~ProtectedTaskManager();

  // common accessors for ui and calc clients
  void SetGlidePolar(const GlidePolar &glide_polar);

  gcc_pure
  const OrderedTaskSettings GetOrderedTaskSettings() const;

  gcc_pure
  WaypointPtr GetActiveWaypoint() const;

  void IncrementActiveTaskPoint(int offset);
  void IncrementActiveTaskPointArm(int offset);

  bool DoGoto(WaypointPtr &&wp);

  bool DoGoto(const WaypointPtr &wp) {
    return DoGoto(WaypointPtr(wp));
  }

  gcc_malloc
  OrderedTask* TaskClone() const;

  /**
   * Copy task into this task
   *
   * @param other OrderedTask to copy
   * @return True if this task changed
   */
  bool TaskCommit(const OrderedTask& that);

  /**
   * Throws std::runtime_error on error.
   */
  void TaskSave(Path path);

  /**
   * Throws std::runtime_error on error.
   */
  void TaskSaveDefault();

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

  void ResetTask();
};

#endif
