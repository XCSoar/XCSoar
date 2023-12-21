// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/NonCopyable.hpp"
#include "Stats/TaskStats.hpp"
#include "Stats/CommonStats.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "TaskBehaviour.hpp"
#include "Waypoint/Ptr.hpp"
#include "util/Compiler.h"

#include <memory>

class AbstractTaskFactory;
class TaskEvents;
class TaskAdvance;
class Waypoints;
class AbstractTask;
class OrderedTask;
class GotoTask;
class AlternateTask;
class AlternateList;
class TaskWaypoint;
class AbortIntersectionTest;
struct RangeAndRadial;

/**
 *  Main interface exposed to clients for providing access to common types
 *  of navigation tasks.  Hides details of these AbstractTasks behind a facade.
 */
class TaskManager: 
  private NonCopyable
{
  GlidePolar glide_polar = GlidePolar::Invalid();

  /**
   * Same as #glide_polar, but with the "safety" MacCready setting
   * applied.
   */
  GlidePolar safety_polar = GlidePolar::Invalid();

  TaskBehaviour task_behaviour;

  const std::unique_ptr<OrderedTask> ordered_task;
  const std::unique_ptr<GotoTask> goto_task;
  const std::unique_ptr<AlternateTask> abort_task;

  TaskType mode = TaskType::NONE;
  AbstractTask* active_task = nullptr;

  TaskStats null_stats;

  CommonStats common_stats;

public:
  /**
   * Constructor for task manager
   *
   * @param wps Waypoint system for use by AbortTask
   *
   * @return Initialised object
   */
  TaskManager(const TaskBehaviour &task_behaviour, const Waypoints &wps) noexcept;
  ~TaskManager() noexcept;

  void SetTaskEvents(TaskEvents &_task_events) noexcept;

  /**
   * Returns a reference to the OrderedTask instance, even if it is
   * invalid or inactive.
   */
  const OrderedTask &GetOrderedTask() const noexcept {
    return *ordered_task;
  }

  /**
   * Increments active taskpoint sequence for active task
   *
   * @param offset Offset value
   */
  void IncrementActiveTaskPoint(int offset) noexcept;

  /**
   * Sets active taskpoint sequence for active task
   *
   * @param index Sequence number of task point
   */
  void SetActiveTaskPoint(unsigned index) noexcept;

  /**
   * Accessor for active taskpoint sequence for active task
   *
   * @return Sequence number of task point
   */
  [[gnu::pure]]
  unsigned GetActiveTaskPointIndex() const noexcept;

  /**
   * Get a random point in the task OZ (for testing simulation route)
   *
   * @param index Index sequence of task point
   * @param mag proportional magnitude of error from center (0-1)
   *
   * @return Location of point
   */
  GeoPoint RandomPointInTask(const unsigned index,
                             double mag = 1) const noexcept;

  /**
   * Retrieve a copy of the task alternates
   *
   * @param index Index sequence of alternate
   *
   * @return Vector of alternates
   */
  [[gnu::const]]
  const AlternateList &GetAlternates() const noexcept;

  /** Reset the tasks (as if never flown) */
  void Reset() noexcept;

  /** Set active task to abort mode. */
  void Abort() noexcept {
    SetMode(TaskType::ABORT);
  }

  /**
   * Sets active task to ordered task (or goto if none exists) after
   * goto or aborting.
   */
  void Resume() noexcept {
    SetMode(TaskType::ORDERED);
  }

  /**
   * Sets active task to go to mode, to specified waypoint
   *
   * @param wp Waypoint to go to
   * @return True if successful
   */
  bool DoGoto(WaypointPtr &&wp) noexcept;

  /**
   * Updates internal state of task given new aircraft.
   * Only essential calculations are performed here;
   * other calculations and housekeeping may be performed
   * by update_idle
   *
   * @param state_now Current aircraft state
   * @param state_last Aircraft state at last update
   * @return True if internal state changed
   */
  bool Update(const AircraftState &state_now, 
              const AircraftState &state_last) noexcept;

  /**
   * Updates internal state of task to produce
   * auxiliary information or to perform slow house-keeping
   * functions that are non-essential.
   *
   * @param state Current aircraft state
   *
   * @return True if internal state changed
   */
  bool UpdateIdle(const AircraftState &state) noexcept;

  /** 
   * Update auto MC.  Internally uses TaskBehaviour to determine settings
   * 
   * @param state_now Current state
   * @param fallback_mc MC value (m/s) to use if algorithm fails or not active
   * 
   * @return True if MC updated
   */
  bool UpdateAutoMC(const AircraftState& state_now, double fallback_mc) noexcept;

  /**
   * Accessor for statistics of active task
   *
   * @return Statistics of active task
   */
  [[gnu::pure]]
  const TaskStats &GetStats() const noexcept;

  /**
   * Accessor for common statistics
   *
   * @return Statistics
   */
  [[gnu::pure]]
  const CommonStats &GetCommonStats() const noexcept {
    return common_stats;
  }

  /**
   * Convenience function, determines whether stats are valid
   *
   * @return True if stats valid
   */
  [[gnu::pure]]
  bool StatsValid() const noexcept {
    return GetStats().task_valid;
  }

  [[gnu::pure]]
  const AbstractTask *GetActiveTask() const noexcept {
    return active_task;
  }

  /**
   * Check whether ordered task is valid
   *
   * @return True if task is valid
   */
  [[gnu::pure]]
  bool CheckOrderedTask() const noexcept;

  /**
   * Check whether active task is valid
   *
   * @return True if task is valid
   */
  [[gnu::pure]]
  bool CheckTask() const noexcept;

  /**
   * Accessor for factory system for constructing tasks
   *
   * @return Factory
   */
  [[gnu::pure]]
  AbstractTaskFactory &GetFactory() const noexcept;

  /**
   * Set type of task factory to be used for constructing tasks
   *
   * @param _factory Type of task
   */
  void SetFactory(const TaskFactoryType _factory) noexcept;

  /**
   * Create a clone of the task. 
   * Caller is responsible for destruction.
   *
   * @param te Task events
   * @param tb Task behaviour
   * @param gp Glide Polar
   *
   * @return Initialised object
   */
  std::unique_ptr<OrderedTask> Clone(const TaskBehaviour &tb) const noexcept;

  /**
   * Copy task into this task
   *
   * @param that OrderedTask to copy
   * @return True if this task changed
   */
  bool Commit(const OrderedTask &that) noexcept;

  /**
   * Accessor for task advance system
   *
   * @return Task advance mechanism
   */
  TaskAdvance &SetTaskAdvance() noexcept;

  /**
   * Access active task mode
   *
   * @return Active task mode
   */
  [[gnu::pure]]
  TaskType GetMode() const noexcept {
    return mode;
  }

  /**
   * Determine if the active mode is a particular mode
   *
   * @param the_mode Mode to compare against
   *
   * @return True if modes match
   */
  [[gnu::pure]]
  bool IsMode(const TaskType _mode) const noexcept {
    return mode == _mode;
  }

  /**
   * Retrieves glide polar used by task system
   *
   * @return Reference to glide polar
   */
  [[gnu::pure]]
  const GlidePolar &GetGlidePolar() const noexcept {
    return glide_polar;
  }

  /**
   * Update glide polar used by task system
   *
   * @param glide_polar The polar to set to
   */
  void SetGlidePolar(const GlidePolar &glide_polar) noexcept;

  /**
   * Retrieve copy of safety glide polar used by task system
   *
   * @return Copy of glide polar
   */
  [[gnu::pure]]
  const GlidePolar &GetSafetyPolar() const noexcept {
    return safety_polar;
  }

  /**
   * Return a reference to the polar that should be used for reach
   * calculations.
   */
  [[gnu::pure]]
  const GlidePolar &GetReachPolar() const noexcept {
    switch (task_behaviour.route_planner.reach_polar_mode) {
    case RoutePlannerConfig::Polar::TASK:
      return glide_polar;

    case RoutePlannerConfig::Polar::SAFETY:
      return safety_polar;
    }

    gcc_unreachable();
    assert(false);
    return glide_polar;
  }

  /**
   * Accessor to get target location of specified tp
   *
   * @param TPindex index of tp in task
   * @return the target location or GeoPoint::Invalid() if that's not
   * a valid AAT point
   */
 const GeoPoint GetLocationTarget(const unsigned index) const noexcept;

  /**
   * Accessor for locked state of target of specified tp
   *
   * @param TPindex index of tp in task
   *
   * @return True if target is locked or tp location if has no target
   */
 bool TargetIsLocked(const unsigned index) const noexcept;

  /**
   * Set target location explicitly of specified tp
   *
   * @param TPindex index of tp in task
   * @param loc Location of new target
   * @param override_lock If false, won't set the target if it is locked
   */
 bool SetTarget(const unsigned index, const GeoPoint &loc,
                const bool override_lock) noexcept;

  /**
   * Set target location from a range and radial
   * referenced on the bearing from the previous target
   * used by dlgTarget
   */
 bool SetTarget(const unsigned index, RangeAndRadial rar) noexcept;

  /**
   * Lock/unlock the target from automatic shifts of specified tp
   *
   * @param TPindex index of tp in task
   * @param do_lock Whether to lock the target
   */
 bool TargetLock(const unsigned index, bool do_lock) noexcept;

  /** 
   * Copy TaskBehaviour to this task
   * 
   * @param behaviour Value to set
   */
  void SetTaskBehaviour(const TaskBehaviour& behaviour) noexcept;

  /** 
   * Retrieve the #OrderedTaskSettings used by the OrderedTask
   * 
   * @return #OrderedTaskSettings reference
   */
  void SetOrderedTaskSettings(const OrderedTaskSettings &otb) noexcept;

  /** 
   * Retrieve task behaviour
   * 
   * @return Reference to task behaviour
   */
  const TaskBehaviour &GetTaskBehaviour() const noexcept {
    return task_behaviour;
  }

  /**
   * Set external test function to be used for additional intersection tests
   */
  void SetIntersectionTest(AbortIntersectionTest *test) noexcept;

  /**
   * When called on takeoff, will create a goto task to the nearest waypoint if
   * no other task is active.
   * Caller is responsible for ensuring the waypoint database already has an
   * appropriate waypoint within 1000m of the takeoff location.
   */
  void TakeoffAutotask(const GeoPoint &ref, double terrain_alt) noexcept;

  void UpdateCommonStatsTask() noexcept;

  void ResetTask() noexcept;

private:
  TaskType SetMode(const TaskType mode) noexcept;

  void UpdateCommonStats(const AircraftState &state) noexcept;
  void UpdateCommonStatsTimes(const AircraftState &state) noexcept;
  void UpdateCommonStatsWaypoints(const AircraftState &state) noexcept;
  void UpdateCommonStatsPolar(const AircraftState &state) noexcept;
};
