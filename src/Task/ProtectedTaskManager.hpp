// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Guard.hpp"
#include "time/RoughTime.hpp"
#include "Engine/Task/Unordered/AbortIntersectionTest.hpp"
#include "Engine/Waypoint/Ptr.hpp"

#include <memory>

struct AGeoPoint;
struct TaskBehaviour;
struct OrderedTaskSettings;
class Path;
class GlidePolar;
class ProtectedRoutePlanner;
class OrderedTask;
class TaskManager;

class ReachIntersectionTest: public AbortIntersectionTest {
  const ProtectedRoutePlanner *route = nullptr;

public:
  void SetRoute(const ProtectedRoutePlanner *_route) noexcept {
    route = _route;
  }

  virtual bool Intersects(const AGeoPoint &destination) const noexcept;
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
  ProtectedTaskManager(TaskManager &_task_manager, const TaskBehaviour &tb) noexcept;

  ~ProtectedTaskManager() noexcept;

  // common accessors for ui and calc clients
  void SetGlidePolar(const GlidePolar &glide_polar) noexcept;

  [[gnu::pure]]
  const OrderedTaskSettings GetOrderedTaskSettings() const noexcept;

  void SetStartTimeSpan(const RoughTimeSpan &open_time_span) noexcept;

  [[gnu::pure]]
  WaypointPtr GetActiveWaypoint() const noexcept;

  void IncrementActiveTaskPoint(int offset) noexcept;
  void IncrementActiveTaskPointArm(int offset) noexcept;

  bool DoGoto(WaypointPtr &&wp) noexcept;

  bool DoGoto(const WaypointPtr &wp) noexcept {
    return DoGoto(WaypointPtr(wp));
  }

  std::unique_ptr<OrderedTask> TaskClone() const noexcept;

  /**
   * Copy task into this task
   *
   * @param other OrderedTask to copy
   * @return True if this task changed
   */
  bool TaskCommit(const OrderedTask &that) noexcept;

  /**
   * Throws std::runtime_error on error.
   */
  void TaskSave(Path path);

  /**
   * Throws std::runtime_error on error.
   */
  void TaskSaveDefault();

  /** Reset the tasks (as if never flown) */
  void Reset() noexcept;

  /**
   * Lock/unlock the target from automatic shifts of specified tp
   *
   * @param index index of tp in task
   * @param do_lock Whether to lock the target
   */
  bool TargetLock(const unsigned index, bool do_lock) noexcept;

  void SetRoutePlanner(const ProtectedRoutePlanner *_route) noexcept;

  [[gnu::pure]]
  short GetTerrainBase() const noexcept;

  void ResetTask() noexcept;
};
