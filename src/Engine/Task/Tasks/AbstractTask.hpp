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
#ifndef ABSTRACTTASK_H
#define ABSTRACTTASK_H

#include "TaskInterface.hpp"
#include "Task/TaskStats/TaskStats.hpp"
#include "Util/Filter.hpp"

class TaskPointConstVisitor;
class TaskPointVisitor;
class AbortTask;
class TaskBehaviour;
class TaskEvents;
class GlidePolar;

/**
 * Abstract base class for actual navigatable tasks.
 * Ensures all tasks have common interfaces and allows for
 * re-use of code for various logic elements.
 */
class AbstractTask: 
  public TaskInterface 
{
  friend class PrintHelper;

protected:
  unsigned activeTaskPoint; /**< task point sequence index */
  unsigned activeTaskPoint_last; /**< task point sequence index at last update*/
  TaskStats stats; /**< statistics of this task */
  TaskStatsComputer stats_computer;
  TaskEvents &task_events; /**< reference to task events (feedback) */
  const TaskBehaviour &task_behaviour; /**< reference to task behaviour (settings) */
  const GlidePolar &glide_polar; /**< reference to global glide polar */

private:
  Filter mc_lpf; /**< low pass filter on best MC calculations */
  Filter ce_lpf; /**< low pass filter on cruise efficiency calculations */
  Filter em_lpf; /**< low pass filter on effective MC calculations */

  bool trigger_auto; /**< whether auto MC has been triggered (above final glide) */

public:
  /** 
   * Base constructor.  Sets time constants of Best Mc and cruise efficiency
   * low pass filters.
   * 
   * @param te Task events callback class (shared among all tasks) 
   * @param tb Global task behaviour settings
   * @param gp Global glide polar used for navigation calculations
   * 
   * @return Initialised object
   */
  AbstractTask(enum type _type, TaskEvents &te,
               const TaskBehaviour &tb,
               const GlidePolar &gp);
  /** 
   * Reset the task (as if never flown)
   * 
   */
  virtual void reset();

  /** 
   * Reset the auto Mc calculator
   * 
   */
  void reset_auto_mc();

/** 
 * Retrieves the active task point sequence.
 * 
 * @return Index of active task point sequence
 */
  gcc_pure
  unsigned getActiveTaskPointIndex() const;

/** 
 * Accessor for task statistics for this task
 * 
 * @return Task statistics reference
 */
  gcc_pure
  virtual const TaskStats& get_stats() const {
    return stats;
  }

/** 
 * Test if task has finished.  Used to determine whether
 * or not to continue updating stats.
 * 
 * @return True if task is finished
 */
  gcc_pure
  virtual bool task_finished() const {
    return false;
  }

/** 
 * Test if task has started.  Used to determine whether
 * or not update stats.  Soft starts are defined as when the formal
 * start condition may or may not be satisfied, but the task is evolving
 * anyway.
 *
 * @param soft If true, allow soft starts
 *
 * @return True if task has started
 */
  gcc_pure
  virtual bool task_started(bool soft=false) const {
    return true;
  }

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
    bool update(const AIRCRAFT_STATE &state_now, const AIRCRAFT_STATE &state_last);
    
/** 
 * Update internal states (non-essential) for housework, or where functions are slow
 * and would cause loss to real-time performance.
 * 
 * @param state_now Aircraft state at this time step
 * 
 * @return True if internal state changed
 */
  virtual bool update_idle(const AIRCRAFT_STATE& state_now);

  /** 
   * Update auto MC.  Internally uses TaskBehaviour to determine settings
   * 
   * @param glide_polar a GlidePolar object to be edited
   * @param state_now Current state
   * @param fallback_mc MC value (m/s) to use if algorithm fails or not active
   * 
   * @return True if MC updated
   */
  bool update_auto_mc(GlidePolar &glide_polar,
                      const AIRCRAFT_STATE& state_now,
                      const fixed fallback_mc);

/** 
 * Check if task is valid.  Calls task_event methods on failure.
 * 
 * @return True if task is valid
 */
  gcc_pure
  virtual bool check_task() const = 0;

  /**
   * Return required arrival height of final point in task
   */
  gcc_pure
  virtual fixed get_finish_height() const = 0;

  /** 
   * Find location of center of task (for rendering purposes)
   * 
   * @param fallback_location Location to use if no valid task
   * 
   * @return Location of center of task
   */
  gcc_pure
  virtual GeoPoint get_task_center(const GeoPoint& fallback_location) const = 0;

  /** 
   * Find approximate radius of task from center to edge (for rendering purposes)
   * 
   * @param fallback_location Location to use if no valid task
   * 
   * @return Radius (m) from center to edge of task
   */
  gcc_pure
  virtual fixed get_task_radius(const GeoPoint& fallback_location) const = 0;
    
protected:

/** 
 * Pure abstract method to be defined for concrete task classes to update internal
 * states when aircraft state advances.
 * 
 * @param state_now Aircraft state at this time step
 * @param full_update Force update due to task state change
 *
 * @return True if internal state changes
 */
  virtual bool update_sample(const AIRCRAFT_STATE &state_now, 
                             const bool full_update) = 0;

/** 
 * Pure abstract method to be defined for concrete task classes to test whether
 * (and how) transitioning into/out of task points should occur, typically
 * according to task_advance mechanism.  This also may call the task_event callbacks.
 * 
 * @param state_now Aircraft state at this time step
 * @param state_last Aircraft state at previous time step
 * 
 * @return True if transition occurred
 */
  virtual bool check_transitions(const AIRCRAFT_STATE& state_now, 
                                 const AIRCRAFT_STATE& state_last) = 0;
  
/** 
 * Calculate/search for best MC, being the highest MC value to produce a
 * pure glide solution for the remainder of the task.
 * 
 * @param state_now Aircraft state
 * @param best Best MC value found (m/s)
 *
 * @return true if solution is valid
 */
  virtual bool calc_mc_best(const AIRCRAFT_STATE &state_now,
                            fixed &best) const = 0;

/** 
 * Calculate virtual sink rate of aircraft that allows a pure glide solution
 * for the remainder of the task.  Glide is performed according to Mc theory
 * speed with the current glide polar, neglecting effect of virtual sink rate.
 * 
 * @param state_now Aircraft state
 * 
 * @return Sink rate of aircraft (m/s)
 */
  virtual fixed calc_glide_required(const AIRCRAFT_STATE &state_now) const = 0;

/** 
 * Calculate cruise efficiency for the travelled part of the task.
 * This is the ratio of the achieved inter-thermal cruise speed to that
 * predicted by MacCready theory with the current glide polar.
 * 
 * Defaults to 1.0 for non-ordered tasks, since non-ordered tasks have no
 * task start time.
 *
 * @param state_now Aircraft state
 * @param value Output cruise efficiency value (0-)
 *
 * @return True if cruise efficiency is updated
 */
  gcc_pure
  virtual bool calc_cruise_efficiency(const AIRCRAFT_STATE &state_now, fixed &val) const {
    val= fixed_one;
    return true;
  }

/** 
 * Calculate effective MacCready for the travelled part of the task.
 * 
 * Defaults to MC for non-ordered tasks, since non-ordered tasks have no
 * task start time.
 *
 * @param state_now Aircraft state
 * @param val Output calculated effective mc
 * 
 * @return True if cruise efficiency is updated
 */
  gcc_pure
  virtual bool calc_effective_mc(const AIRCRAFT_STATE &state_now, fixed& val) const;

/** 
 * Optimise target ranges (for adjustable tasks) to produce an estimated
 * time remaining with the current glide polar, equal to a target value.
 *
 * For non-ordered tasks, this doesn't do anything and returns 0.0
 * 
 * @param state_now Aircraft state
 * @param t_target Desired time for remainder of task (s)
 * 
 * @return Target range parameter (0-1)
 */
  gcc_pure
  virtual fixed calc_min_target(const AIRCRAFT_STATE &state_now, 
                                const fixed t_target) {
    return fixed_zero;
  };

/** 
 * Calculate angle from aircraft to destination of current leg (height above taskpoint divided
 * by distance to go).  
 * 
 * @param state_now Aircraft state
 * 
 * @return Gradient angle of remainder of task
 */
  gcc_pure
  fixed leg_gradient(const AIRCRAFT_STATE &state_now) const;

/** 
 * Calculate angle from aircraft to remainder of task (height above finish divided
 * by distance to go).  
 * 
 * @param state_now Aircraft state
 * 
 * @return Gradient angle of remainder of task
 */
  gcc_pure
  virtual fixed calc_gradient(const AIRCRAFT_STATE &state_now) const = 0;

/** 
 * Calculate task start time.  Default behaviour is current time, to be used
 * for non-ordered tasks.
 * 
 * @param state_now Aircraft state
 * 
 * @return Time (s) of start of task
 */
  virtual fixed scan_total_start_time(const AIRCRAFT_STATE &state_now) = 0;

/** 
 * Calculate leg start time.  Default behaviour is current time, to be used
 * for non-ordered tasks.
 * 
 * @param state_now Aircraft state
 * 
 * @return Time (s) of start of leg
 */
  virtual fixed scan_leg_start_time(const AIRCRAFT_STATE &state_now) = 0;

/** 
 * Calculate distance of nominal task (sum of distances from each
 * leg's consecutive reference point to reference point for entire task).
 * 
 * @return Distance (m) of nominal task
 */ 
  virtual fixed scan_distance_nominal() = 0;

/** 
 * Calculate distance of planned task (sum of distances from each leg's
 * achieved/scored reference points respectively for prior task points,
 * and targets or reference points for active and later task points).
 * 
 * @return Distance (m) of planned task
 */
  virtual fixed scan_distance_planned() = 0;

/** 
 * Calculate distance of planned task (sum of distances from aircraft to
 * current target/reference and for later task points from each leg's
 * targets or reference points).
 * 
 * @param ref Location of aircraft
 * 
 * @return Distance (m) remaining in the planned task
 */ 
  virtual fixed scan_distance_remaining(const GeoPoint &ref) = 0;

/** 
 * Calculate scored distance of achieved part of task.
 * 
 * @param ref Location of aircraft
 * 
 * @return Distance (m) achieved adjusted for scoring
 */
  virtual fixed scan_distance_scored(const GeoPoint &ref) = 0;

/** 
 * Calculate distance of achieved part of task.
 * For previous taskpoints, the sum of distances of maximum distance
 * points; for current, the distance from previous max distance point to
 * the aircraft.
 * 
 * @param ref Location of aircraft
 * 
 * @return Distance (m) achieved
 */
  virtual fixed scan_distance_travelled(const GeoPoint &ref) = 0;

/** 
 * Calculate maximum and minimum distances for task, achievable
 * from the current aircraft state (assuming active taskpoint does not retreat). 
 * 
 * @param ref Aircraft location
 * @param full Perform full search (if task state has changed)
 * @param dmin Minimum distance (m) achievable of task
 * @param dmax Maximum distance (m) achievable of task
 */
  virtual void scan_distance_minmax(const GeoPoint &ref, 
                                    bool full,
                                    fixed *dmin, fixed *dmax) = 0;

/** 
 * Calculate glide result for remainder of task
 * 
 * @param state_now Aircraft state
 * @param polar Glide polar used for calculation
 * @param total Glide result accumulated for total remaining task
 * @param leg Glide result for current leg of task
 */
  virtual void glide_solution_remaining(const AIRCRAFT_STATE &state_now, 
                                        const GlidePolar &polar,
                                        GlideResult &total,
                                        GlideResult &leg) = 0;

/** 
 * Calculate glide result from start of task to current state
 * 
 * @param state_now Aircraft state
 * @param total Glide result accumulated for total travelled task
 * @param leg Glide result for current leg of task
 */
  virtual void glide_solution_travelled(const AIRCRAFT_STATE &state_now, 
                                        GlideResult &total,
                                        GlideResult &leg) = 0;

/** 
 * Calculate glide result from start of task to finish, and from this
 * calculate the effective position of the aircraft along the task based
 * on the remaining time.  This system therefore allows effective speeds
 * to be calculated which take into account the time value of height.
 * 
 * @param state_now Aircraft state
 * @param total Glide result accumulated for total task
 * @param leg Glide result for current leg of task
 * @param total_remaining_effective 
 * @param leg_remaining_effective 
 * @param total_t_elapsed Total planned task time (s)
 * @param leg_t_elapsed Leg planned task time (s)
 */
  virtual void glide_solution_planned(const AIRCRAFT_STATE &state_now,
                                      GlideResult &total,
                                      GlideResult &leg,
                                      DistanceStat &total_remaining_effective,
                                      DistanceStat &leg_remaining_effective,
                                      const fixed total_t_elapsed,
                                      const fixed leg_t_elapsed) = 0;

  /**
   * Determines whether the task has adjustable targets
   */
  gcc_pure
  virtual bool has_targets() const = 0;

  /**
   * Determines whether this task is scored
   */
  gcc_pure
  virtual bool is_scored() const = 0;

protected:
/** 
 * Updates distance calculations and values in the statistics.
 * This is protected rather than private so concrete tasks can 
 * call this when they know the task has been modified.
 * 
 * @param location Location of observer
 * @param full_update Whether all calculations or minimal ones to be performed
 */
  virtual void update_stats_distances(const GeoPoint &location,
                                      const bool full_update);

private:
  void update_glide_solutions(const AIRCRAFT_STATE &state);
  void update_stats_times(const AIRCRAFT_STATE &);
  void update_stats_speeds(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&);
  void update_stats_glide(const AIRCRAFT_STATE &state);
  void update_flight_mode();

public:

/** 
 * Accept a const task point visitor; makes the visitor visit
 * all TaskPoint in the task
 * 
 * @param visitor Visitor to accept
 * @param reverse Perform scan in reverse sequence
 */
  virtual void tp_CAccept(TaskPointConstVisitor& visitor, const bool reverse=false) const = 0;

  /**
   * Accept a (const) task point visitor; makes the visitor visit
   * all optional start points in the task
   *
   * @param visitor Visitor to accept
   * @param reverse Visit task points in reverse order
   */
  virtual void sp_CAccept(TaskPointConstVisitor& visitor, const bool reverse=false) const = 0;
};
#endif //ABSTRACTTASK_H
