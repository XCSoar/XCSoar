/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "Factory/ValidationError.hpp"
#include "Stats/TaskStats.hpp"
#include "Computer/TaskStatsComputer.hpp"
#include "TaskBehaviour.hpp"
#include "Math/Filter.hpp"

class TaskPointConstVisitor;
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
protected:
  /** task point sequence index */
  unsigned active_task_point = 0;
  /** statistics of this task */
  TaskStats stats;
  TaskStatsComputer stats_computer;
  /** reference to task events (feedback) */
  TaskEvents *task_events = nullptr;

  /** settings */
  TaskBehaviour task_behaviour;

  /**
   * Setting this flag enforces a full Update() in the next
   * #CalculationThread iteration.  Set it when the task has been
   * edited.
   */
  bool force_full_update = true;

private:
  /** low pass filter on best MC calculations */
  Filter mc_lpf{8};
  /** low pass filter on cruise efficiency calculations */
  Filter ce_lpf{60};
  /** low pass filter on effective MC calculations */
  Filter em_lpf{60};

  /**
   * True when #mc_lpf has been initialised.
   */
  bool mc_lpf_valid = false;

public:
  /** 
   * Base constructor.  Sets time constants of Best Mc and cruise efficiency
   * low pass filters.
   * 
   * @param tb Global task behaviour settings
   */
  AbstractTask(TaskType _type, const TaskBehaviour &tb) noexcept;

  /**
   * Set the handler for task events.  This method may be called only
   * once.
   */
  void SetTaskEvents(TaskEvents &_task_events) noexcept {
    assert(task_events == NULL);

    task_events = &_task_events;
  }

  /** Reset the task (as if never flown) */
  virtual void Reset() noexcept;

  /** Reset the auto Mc calculator */
  void ResetAutoMC() noexcept;

  void SetTaskBehaviour(const TaskBehaviour &tb) noexcept {
    task_behaviour = tb;
  }

  /** 
   * Retrieves the active task point sequence.
   * 
   * @return Index of active task point sequence
   */
  unsigned GetActiveTaskPointIndex() const noexcept {
    return active_task_point;
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
  [[gnu::pure]]
  virtual bool TaskStarted([[maybe_unused]] bool soft = false) const noexcept {
    return true;
  }

  const TaskStats &GetStats() const noexcept {
    return stats;
  }

  /** 
   * Update auto MC.  Internally uses TaskBehaviour to determine settings
   * 
   * @param glide_polar a GlidePolar object to be edited
   * @param state_now Current state
   * @param fallback_mc MC value (m/s) to use if algorithm fails or not active
   * 
   * @return True if MC updated
   */
  bool UpdateAutoMC(GlidePolar &glide_polar, const AircraftState &state_now,
                    double fallback_mc) noexcept;

  /**
   * Check if task is valid.
   */
  [[gnu::pure]]
  virtual TaskValidationErrorSet CheckTask() const noexcept = 0;

protected:
  /**
   * Pure abstract method to be defined for concrete task classes to update
   * internal states when aircraft state advances.
   *
   * @param state_now Aircraft state at this time step
   * @param full_update Force update due to task state change
   *
   * @return True if internal state changes
   */
  virtual bool UpdateSample(const AircraftState &state_now,
                            const GlidePolar &glide_polar,
                            const bool full_update) noexcept = 0;

  /**
   * Pure abstract method to be defined for concrete task classes to test
   * whether (and how) transitioning into/out of task points should occur,
   * typically according to task_advance mechanism.
   * This also may call the task_event callbacks.
   *
   * @param state_now Aircraft state at this time step
   * @param state_last Aircraft state at previous time step
   *
   * @return True if transition occurred
   */
  virtual bool CheckTransitions(const AircraftState &state_now,
                                const AircraftState &state_last) noexcept = 0;

  /**
   * Calculate/search for best MC, being the highest MC value to produce a
   * pure glide solution for the remainder of the task.
   *
   * @param state_now Aircraft state
   * @param best Best MC value found (m/s)
   *
   * @return true if solution is valid
   */
  virtual bool CalcBestMC(const AircraftState &state_now,
                          const GlidePolar &glide_polar,
                          double &best) const noexcept = 0;

  /**
   * Calculate virtual sink rate of aircraft that allows a pure glide solution
   * for the remainder of the task.  Glide is performed according to Mc theory
   * speed with the current glide polar, neglecting effect of virtual sink rate.
   *
   * @param state_now Aircraft state
   *
   * @return Sink rate of aircraft (m/s)
   */
  virtual double CalcRequiredGlide(const AircraftState &state_now,
                                   const GlidePolar &glide_polar) const noexcept = 0;

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
  [[gnu::pure]]
  virtual bool CalcCruiseEfficiency([[maybe_unused]] const AircraftState &state_now,
                                    [[maybe_unused]] const GlidePolar &glide_polar,
                                    double &val) const noexcept {
    val = 1;
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
  [[gnu::pure]]
  virtual bool CalcEffectiveMC(const AircraftState &state_now,
                               const GlidePolar &glide_polar,
                               double &val) const noexcept;

  /**
   * Calculate angle from aircraft to destination of current leg
   * (height above taskpoint divided by distance to go).
   *
   * @param state_now Aircraft state
   *
   * @return Gradient angle of remainder of task
   */
  [[gnu::pure]]
  double CalcLegGradient(const AircraftState &state_now) const noexcept;

  /**
   * Calculate angle from aircraft to remainder of task
   * (height above finish divided by distance to go).
   *
   * @param state_now Aircraft state
   *
   * @return Gradient angle of remainder of task
   */
  [[gnu::pure]]
  virtual double CalcGradient(const AircraftState &state_now) const noexcept = 0;

  /**
   * Calculate task start time.
   * Default behaviour is current time, to be used for non-ordered tasks.
   *
   * @return Time (s) of start of task or negative value if not available
   */
  virtual TimeStamp ScanTotalStartTime() noexcept = 0;

  /**
   * Calculate leg start time.
   * Default behaviour is current time, to be used for non-ordered tasks.
   *
   * @return Time (s) of start of leg or negative value if not available
   */
  virtual TimeStamp ScanLegStartTime() noexcept = 0;

  /**
   * Calculate distance of nominal task (sum of distances from each
   * leg's consecutive reference point to reference point for entire task).
   *
   * @return Distance (m) of nominal task
   */
  virtual double ScanDistanceNominal() noexcept = 0;

  /**
   * Calculate distance of planned task (sum of distances from each leg's
   * achieved/scored reference points respectively for prior task points,
   * and targets or reference points for active and later task points).
   *
   * @return Distance (m) of planned task
   */
  virtual double ScanDistancePlanned() noexcept = 0;

  /**
   * Calculate distance of planned task (sum of distances from aircraft to
   * current target/reference and for later task points from each leg's
   * targets or reference points).
   *
   * @param ref Location of aircraft
   *
   * @return Distance (m) remaining in the planned task
   */
  virtual double ScanDistanceRemaining(const GeoPoint &ref) noexcept = 0;

  /**
   * Calculate scored distance of achieved part of task.
   *
   * @param ref Location of aircraft
   *
   * @return Distance (m) achieved adjusted for scoring
   */
  virtual double ScanDistanceScored(const GeoPoint &ref) noexcept = 0;

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
  virtual double ScanDistanceTravelled(const GeoPoint &ref) noexcept = 0;

  /**
   * Calculate maximum and minimum distances for task, achievable
   * from the current aircraft state (assuming active taskpoint does not retreat).
   *
   * @param ref Aircraft location
   * @param full Perform full search (if task state has changed)
   * @param dmin Minimum distance (m) achievable of task
   * @param dmax Maximum distance (m) achievable of task
   */
  virtual void ScanDistanceMinMax(const GeoPoint &ref, bool full,
                                  double *dmin, double *dmax) noexcept = 0;

  /**
   * Calculate glide result for remainder of task
   *
   * @param state_now Aircraft state
   * @param polar Glide polar used for calculation
   * @param total Glide result accumulated for total remaining task
   * @param leg Glide result for current leg of task
   */
  virtual void GlideSolutionRemaining(const AircraftState &state_now,
                                      const GlidePolar &polar,
                                      GlideResult &total, GlideResult &leg) noexcept = 0;

  /**
   * Calculate glide result from start of task to current state
   *
   * @param state_now Aircraft state
   * @param total Glide result accumulated for total travelled task
   * @param leg Glide result for current leg of task
   */
  virtual void GlideSolutionTravelled(const AircraftState &state_now,
                                      const GlidePolar &glide_polar,
                                      GlideResult &total, GlideResult &leg) noexcept = 0;

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
  virtual void GlideSolutionPlanned(const AircraftState &state_now,
                                    const GlidePolar &glide_polar,
                                    GlideResult &total, GlideResult &leg,
                                    DistanceStat &total_remaining_effective,
                                    DistanceStat &leg_remaining_effective,
                                    const GlideResult &solution_remaining_total,
                                    const GlideResult &solution_remaining_leg) noexcept = 0;

  /** Determines whether this task is scored */
  [[gnu::pure]]
  virtual bool IsScored() const noexcept = 0;

protected:
  /**
   * Updates distance calculations and values in the statistics.
   * This is protected rather than private so concrete tasks can
   * call this when they know the task has been modified.
   *
   * @param location Location of observer
   * @param full_update Whether all calculations or minimal ones to be performed
   */
  void UpdateStatsDistances(const GeoPoint &location, const bool full_update) noexcept;

private:
  void UpdateGlideSolutions(const AircraftState &state,
                            const GlidePolar &glide_polar) noexcept;

  /**
   * @param time monotonic time of day in seconds or -1 if unknown
   */
  void UpdateStatsTimes(TimeStamp time) noexcept;
  void UpdateStatsSpeeds(TimeStamp time) noexcept;
  void UpdateStatsGlide(const AircraftState &state,
                        const GlidePolar &glide_polar) noexcept;
  void UpdateFlightMode() noexcept;

public:
  /**
   * Accept a const task point visitor; makes the visitor visit
   * all TaskPoint in the task
   *
   * @param visitor Visitor to accept
   * @param reverse Perform scan in reverse sequence
   */
  virtual void AcceptTaskPointVisitor(TaskPointConstVisitor &visitor) const = 0;

public:
  /* virtual methods from class TaskInterface */
  bool Update(const AircraftState &state_now,
              const AircraftState &state_last,
              const GlidePolar &glide_polar) noexcept override;
  bool UpdateIdle(const AircraftState &state_now,
                  const GlidePolar &glide_polar) noexcept override;
};

#endif //ABSTRACTTASK_H
