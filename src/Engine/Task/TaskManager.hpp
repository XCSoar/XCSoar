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

#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "Compiler.h"
#include "Util/NonCopyable.hpp"
#include "Tasks/AlternateTask.hpp"
#include "Tasks/GotoTask.hpp"
#include "Tasks/OrderedTask.hpp"
#include "TaskStats/TaskStats.hpp"
#include "TaskStats/CommonStats.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "TaskBehaviour.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"

class AbstractTaskFactory;
class TaskEvents;
class TaskAdvance;
class Waypoints;

/**
 *  Main interface exposed to clients for providing access to common types
 *  of navigation tasks.  Hides details of these AbstractTasks behind a facade.
 */
class TaskManager: 
  private NonCopyable
{
public:
  /**
   * Enumeration of task modes
   */
  enum TaskMode {
    MODE_NULL = 0,
    MODE_ORDERED,
    MODE_ABORT,
    MODE_GOTO
  };

private:
  GlidePolar glide_polar;

  TaskBehaviour task_behaviour;
  OrderedTask task_ordered;
  GotoTask task_goto;
  AlternateTask task_abort;

  TaskMode mode;
  AbstractTask* active_task;

  TaskStats null_stats;

  CommonStats common_stats;

public:
  friend class Serialiser;
  friend class PrintHelper;

  /**
   * Constructor for task manager
   *
   * @param te Task events callback object
   * @param wps Waypoint system for use by AbortTask
   *
   * @return Initialised object
   */
  TaskManager(TaskEvents &te, const Waypoints &wps);

  /**
   * Returns a reference to the OrderedTask instance, even if it is
   * invalid or inactive.
   */
  const OrderedTask &GetOrderedTask() const {
    return task_ordered;
  }

  /**
   * Increments active taskpoint sequence for active task
   *
   * @param offset Offset value
   */
  void IncrementActiveTaskPoint(int offset);

  /**
   * Sets active taskpoint sequence for active task
   *
   * @param index Sequence number of task point
   */
  void SetActiveTaskPoint(unsigned index);

  /**
   * Accessor for active taskpoint sequence for active task
   *
   * @return Sequence number of task point
   */
  gcc_pure
  unsigned GetActiveTaskPointIndex() const;

  /**
   * Accessor of current task point of active task
   *
   * @return TaskPoint of active task point, and 0 if no active task
   */
  gcc_pure
  TaskWaypoint* GetActiveTaskPoint() const;

  /**
   * Get a random point in the task OZ (for testing simulation route)
   *
   * @param index Index sequence of task point
   * @param mag proportional magnitude of error from center (0-1)
   *
   * @return Location of point
   */
  GeoPoint RandomPointInTask(const unsigned index,
                             const fixed mag = fixed_one) const;

  /**
   * Retrieve a copy of the task alternates
   *
   * @param index Index sequence of alternate
   *
   * @return Vector of alternates
   */
  const AbortTask::AlternateVector &GetAlternates() const {
    return task_abort.getAlternates();
  }

  /** Reset the tasks (as if never flown) */
  void Reset();

  /** Set active task to abort mode. */
  void Abort() {
    SetMode(MODE_ABORT);
  }

  /**
   * Sets active task to ordered task (or goto if none exists) after
   * goto or aborting.
   */
  void Resume() {
    SetMode(MODE_ORDERED);
  }

  /**
   * Sets active task to go to mode, to specified waypoint
   *
   * @param wp Waypoint to go to
   * @return True if successful
   */
  bool DoGoto(const Waypoint & wp);

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
              const AircraftState &state_last);

  /**
   * Updates internal state of task to produce
   * auxiliary information or to perform slow house-keeping
   * functions that are non-essential.
   *
   * @param state Current aircraft state
   *
   * @return True if internal state changed
   */
  bool UpdateIdle(const AircraftState &state);

  /** 
   * Update auto MC.  Internally uses TaskBehaviour to determine settings
   * 
   * @param state_now Current state
   * @param fallback_mc MC value (m/s) to use if algorithm fails or not active
   * 
   * @return True if MC updated
   */
  bool UpdateAutoMC(const AircraftState& state_now, const fixed fallback_mc);

  /**
   * Accessor for statistics of active task
   *
   * @return Statistics of active task
   */
  gcc_pure
  const TaskStats& GetStats() const;

  /**
   * Accessor for common statistics
   *
   * @return Statistics
   */
  gcc_pure
  const CommonStats& GetCommonStats() const {
    return common_stats;
  }

  /**
   * Convenience function, determines whether stats are valid
   *
   * @return True if stats valid
   */
  gcc_pure
  bool StatsValid() const {
    return GetStats().task_valid;
  }

  gcc_pure
  const AbstractTask *GetActiveTask() const {
    return active_task;
  }

  /**
   * Size of active task
   *
   * @return Number of taskpoints in active task
   */
  gcc_pure
  unsigned TaskSize() const;

  /**
   * Check whether ordered task is valid
   *
   * @return True if task is valid
   */
  gcc_pure
  bool CheckOrderedTask() const {
    return task_ordered.CheckTask();
  }

  /**
   * Check whether active task is valid
   *
   * @return True if task is valid
   */
  gcc_pure
  bool CheckTask() const;

  /**
   * Accessor for factory system for constructing tasks
   *
   * @return Factory
   */
  gcc_pure
  AbstractTaskFactory& GetFactory() const {
    return task_ordered.GetFactory();
  }

  /**
   * Set type of task factory to be used for constructing tasks
   *
   * @param _factory Type of task
   *
   * @return Type of task
   */
  TaskBehaviour::FactoryType SetFactory(const TaskBehaviour::FactoryType _factory) {
    return task_ordered.SetFactory(_factory);
  }

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
  gcc_malloc gcc_pure
  OrderedTask* Clone(TaskEvents &te, const TaskBehaviour &tb,
                     const GlidePolar &gp) const;

  /**
   * Copy task into this task
   *
   * @param that OrderedTask to copy
   * @return True if this task changed
   */
  bool Commit(const OrderedTask& that);

  /**
   * Accessor for task advance system
   *
   * @return Task advance mechanism
   */
  const TaskAdvance &GetTaskAdvance() const {
    return task_ordered.get_task_advance();
  }

  /**
   * Accessor for task advance system
   *
   * @return Task advance mechanism
   */
  TaskAdvance& GetTaskAdvance() {
    return task_ordered.get_task_advance();
  }

  /**
   * Access active task mode
   *
   * @return Active task mode
   */
  gcc_pure
  TaskMode GetMode() const {
    return mode;
  }

  /**
   * Determine if the active mode is a particular mode
   *
   * @param the_mode Mode to compare against
   *
   * @return True if modes match
   */
  gcc_pure
  bool IsMode(const TaskMode _mode) const {
    return mode == _mode;
  }

  /**
   * Retrieves glide polar used by task system
   *
   * @return Reference to glide polar
   */
  gcc_pure
  const GlidePolar &GetGlidePolar() const {
    return glide_polar;
  }

  /**
   * Update glide polar used by task system
   *
   * @param glide_polar The polar to set to
   */
  void SetGlidePolar(const GlidePolar& glide_polar);

  /**
   * Retrieve copy of safety glide polar used by task system
   *
   * @return Copy of glide polar
   */
  gcc_pure
  GlidePolar GetSafetyPolar() const {
    return task_abort.get_safety_polar();
  }

  /**
   * Accesses ordered task start state
   *
   * @return State at task start (or null state if not started)
   */
  AircraftState GetStartState() const {
    return task_ordered.GetStartState();
  }

  /**
   * Accesses ordered task finish state
   *
   * @return State at task finish (or null state if not finished)
   */
  AircraftState GetFinishState() const {
    return task_ordered.GetFinishState();
  }

  /**
   * Return required arrival height of final point in task
   */
  fixed GetFinishHeight() const;

  /** 
   * Find location of center of task (for rendering purposes)
   * 
   * @param fallback_location Location to use if no task active
   * 
   * @return Location of center of task
   */
  GeoPoint GetTaskCenter(const GeoPoint& fallback_location) const;

  /** 
   * Find approximate radius of task from center to edge (for rendering purposes)
   * 
   * @param fallback_location Location to use if no task active
   * 
   * @return Radius (m) from center to edge of task
   */
  fixed GetTaskRadius(const GeoPoint& fallback_location) const;

  /**
   * Check whether observer is within OZ of specified tp
   *
   * @param TPindex index of tp in task
   * @param ref state of aircraft to be checked
   * @parm AATOnly if true then returns false for non-AAT points
   *
   * @return True if reference point is inside sector
   */
 bool IsInSector(const unsigned index, const AircraftState &ref,
                 const bool AATOnly = true) const;

  /**
   * Accessor to get target location of specified tp
   *
   * @param TPindex index of tp in task
   *
   * @return Target location or fallback_location if TPindex is
   *    invalid or has no target
   */
 const GeoPoint& GetLocationTarget(const unsigned index,
                                   const GeoPoint& fallback_location) const;

  /**
   * Accessor for locked state of target of specified tp
   *
   * @param TPindex index of tp in task
   *
   * @return True if target is locked or tp location if has no target
   */
 bool TargetIsLocked(const unsigned index) const;

  /**
   * Capability of specified TaskPoint to have adjustable range (true for AAT)
   *
   * @param TPindex index of tp in task
   *
   * @return True if task point has a target (can have range set)
   */
 bool HasTarget(const unsigned index) const;

  /**
   * Set target location explicitly of specified tp
   *
   * @param TPindex index of tp in task
   * @param loc Location of new target
   * @param override_lock If false, won't set the target if it is locked
   */
 bool SetTarget(const unsigned index, const GeoPoint &loc,
                const bool override_lock);

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
   * returns position of the target in range / radial format
   * referenced on the bearing from the previous target
   * used by dlgTarget
   *
   * @param &range returns the range [0,1] from center
   * to perimeter of the oz
   *
   * @param &radial returns the angle in degrees of
   * the target in the sector in polar coordinates
   */
 bool GetTargetRangeRadial(const unsigned index, fixed &range,
                           fixed &radial) const;

  /**
   * Lock/unlock the target from automatic shifts of specified tp
   *
   * @param TPindex index of tp in task
   * @param do_lock Whether to lock the target
   */
 bool TargetLock(const unsigned index, bool do_lock);

  /** 
   * Retrieve (const) the OrderedTaskBehaviour used by the OrderedTask
   * 
   * @return Read-only OrderedTaskBehaviour
   */
  const OrderedTaskBehaviour &GetOrderedTaskBehaviour() const {
    return task_ordered.get_ordered_task_behaviour();
  }

  /** 
   * Retrieve the OrderedTaskBehaviour used by the OrderedTask
   * 
   * @return OrderedTaskBehaviour reference
   */
  OrderedTaskBehaviour &GetOrderedTaskBehaviour() {
    return task_ordered.get_ordered_task_behaviour();
  }

  /** 
   * Copy TaskBehaviour to this task
   * 
   * @param behaviour Value to set
   */
  void SetTaskBehaviour(const TaskBehaviour& behaviour);
  
  /** 
   * Retrieve task behaviour
   * 
   * @return Reference to task behaviour
   */
  const TaskBehaviour &GetTaskBehaviour() const {
    return task_behaviour;
  }

  /**
   * Set external test function to be used for additional intersection tests
   */
  void SetIntersectionTest(AbortIntersectionTest *test) {
    task_abort.set_intersection_test(test);
  }

  /**
   * When called on takeoff, will create a goto task to the nearest waypoint if
   * no other task is active.
   * Caller is responsible for ensuring the waypoint database already has an
   * appropriate waypoint within 1000m of the takeoff location.
   */
  void TakeoffAutotask(const GeoPoint &ref, const fixed terrain_alt);

private:
  TaskMode SetMode(const TaskMode mode);

  void UpdateCommonStats(const AircraftState &state);
  void UpdateCommonStatsTimes(const AircraftState &state);
  void UpdateCommonStatsTask(const AircraftState &state);
  void UpdateCommonStatsWaypoints(const AircraftState &state);
  void UpdateCommonStatsPolar(const AircraftState &state);
};

#endif
