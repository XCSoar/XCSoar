/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Compiler.h"

class GlidePolar;
class RoutePlannerGlue;

class ReachIntersectionTest: public AbortIntersectionTest {
public:
  ReachIntersectionTest(): route(NULL) {};
  void SetRoute(const RoutePlannerGlue *_route) {
    route = _route;
  }
  virtual bool Intersects(const AGeoPoint& destination);
private:
  const RoutePlannerGlue *route;
};

/**
 * Facade to task/airspace/waypoints as used by threads,
 * to manage locking
 */
class ProtectedTaskManager: public Guard<TaskManager>
{
protected:
  const TaskBehaviour &task_behaviour;
  TaskEvents &task_events;
  ReachIntersectionTest intersection_test;

  static const TCHAR default_task_path[];

public:
  ProtectedTaskManager(TaskManager &_task_manager, const TaskBehaviour &tb,
                       TaskEvents &te)
    :Guard<TaskManager>(_task_manager),
     task_behaviour(tb), task_events(te)
    {}
  
  ~ProtectedTaskManager();

  // common accessors for ui and calc clients
  void SetGlidePolar(const GlidePolar &glide_polar);

  gcc_pure
  TaskManager::TaskMode_t GetMode() const;

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

  gcc_malloc
  OrderedTask* TaskBlank() const;

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
                                 TaskBehaviour::Factory_t factory);

  gcc_malloc
  OrderedTask* TaskCopy(const OrderedTask& that) const;

  gcc_malloc
  OrderedTask *TaskCreate(const TCHAR *path, const Waypoints *waypoints,
                          unsigned index = 0) const;
  bool TaskSave(const TCHAR* path, const OrderedTask& task);

  /** Reset the tasks (as if never flown) */
  void Reset();

  /**
   * Check whether observer is within OZ of specified tp
   *
   * @param index index of tp in task
   * @param ref state of aircraft to be checked
   *
   * @return True if reference point is inside sector
   */
  gcc_pure
  bool IsInSector(const unsigned index, const AircraftState &ref) const;

  /**
   * Set target location from a range and radial
   * referenced on the bearing from the previous target
   * used by dlgTarget
   *
   * @param range the range [0,1] from center to perimeter
   * of the oz
   *
   * @param radial the angle in degrees of the target
   */
  bool SetTarget(const unsigned index, const fixed range, const fixed radial);

  /**
   * Lock/unlock the target from automatic shifts of specified tp
   *
   * @param index index of tp in task
   * @param do_lock Whether to lock the target
   */
  bool TargetLock(const unsigned index, bool do_lock);

  /**
   * returns copy of name of specified ordered tp
   *
   * @param index index of ordered tp in task
   */
  gcc_pure
  const TCHAR* GetOrderedTaskpointName(const unsigned index) const;

  void SetRoutePlanner(const RoutePlannerGlue *_route);

  short GetTerrainBase() const;
};

#endif
