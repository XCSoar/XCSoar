#ifndef ABSTRACTTASK_H
#define ABSTRACTTASK_H

#include "TaskInterface.hpp"
#include "Task/TaskEvents.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Task/TaskAdvance.hpp"
#include "Task/TaskStats/TaskStats.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Util/Filter.hpp"

class TaskPointVisitor;

/**
 * Abstract base class for actual navigatable tasks.
 * Ensures all tasks have common interfaces and allows for
 * re-use of code for various logic elements.
 */
class AbstractTask : public TaskInterface {
public:
  /** 
   * Base constructor.  Sets time constants of Best Mc and cruise efficiency
   * low pass filters.
   * 
   * @param te Task events callback class (shared among all tasks) 
   * @param tb Global task behaviour settings
   * @param ta Advance mechanism used for advancable tasks
   * @param gp Global glide polar used for navigation calculations
   * 
   * @return Initialised object
   */
  AbstractTask(const TaskEvents &te,
               const TaskBehaviour &tb,
               TaskAdvance &ta,
               GlidePolar &gp): 
    activeTaskPoint(0),
    task_events(te),
    task_advance(ta),
    task_behaviour(tb),
    glide_polar(gp),
    mc_lpf(10.0),
    ce_lpf(60.0),
    trigger_auto(false)
  {};

  /** 
   * Reset the task (as if never flown)
   * 
   */
  virtual void reset();

/** 
 * Retrieves the active task point sequence.
 * 
 * @return Index of active task point sequence
 */
  unsigned getActiveTaskPointIndex() const;

/** 
 * Accessor for task statistics for this task
 * 
 * @return Task statistics reference
 */
    virtual const TaskStats& get_stats() const {
      return stats;
    }

/** 
 * Test if task has finished.  Used to determine whether
 * or not to continue updating stats.
 * 
 * @return True if task is finished
 */
  virtual bool task_finished() const {
    return false;
  }

/** 
 * Test if task has started.  Used to determine whether
 * or not update stats.
 * 
 * @return True if task has started
 */
  virtual bool task_started() const {
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
 * 
 * @return Best MC value found (m/s)
 */
  virtual double calc_mc_best(const AIRCRAFT_STATE &state_now);

/** 
 * Calculate virtual sink rate of aircraft that allows a pure glide solution
 * for the remainder of the task.  Glide is performed according to Mc theory
 * speed with the current glide polar, neglecting effect of virtual sink rate.
 * 
 * @param state_now Aircraft state
 * 
 * @return Sink rate of aircraft (m/s)
 */
  virtual double calc_glide_required(const AIRCRAFT_STATE &state_now);

/** 
 * Calculate cruise efficiency for the travelled part of the task.
 * This is the ratio of the achieved inter-thermal cruise speed to that
 * predicted by MacCready theory with the current glide polar.
 * 
 * Defaults to 1.0 for non-ordered tasks, since non-ordered tasks have no
 * task start time.
 *
 * @param state_now Aircraft state
 * 
 * @return Cruise efficiency (0-1)
 */
  virtual double calc_cruise_efficiency(const AIRCRAFT_STATE &state_now) {
    return 1.0;
  }

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
  virtual double calc_min_target(const AIRCRAFT_STATE &state_now, 
                                 const double t_target) {
    return 0.0;
  };

/** 
 * Calculate angle from aircraft to remainder of task (height above finish divided
 * by distance to go).  
 * 
 * @param state_now Aircraft state
 * 
 * @return Gradient angle of remainder of task
 */
  virtual double calc_gradient(const AIRCRAFT_STATE &state_now);

/** 
 * Calculate task start time.  Default behaviour is current time, to be used
 * for non-ordered tasks.
 * 
 * @param state_now Aircraft state
 * 
 * @return Time (s) of start of task
 */
  virtual double scan_total_start_time(const AIRCRAFT_STATE &state_now);

/** 
 * Calculate leg start time.  Default behaviour is current time, to be used
 * for non-ordered tasks.
 * 
 * @param state_now Aircraft state
 * 
 * @return Time (s) of start of leg
 */
  virtual double scan_leg_start_time(const AIRCRAFT_STATE &state_now);

/** 
 * Calculate distance of nominal task (sum of distances from each
 * leg's consecutive reference point to reference point for entire task).
 * 
 * @return Distance (m) of nominal task
 */ 
  virtual double scan_distance_nominal();

/** 
 * Calculate distance of planned task (sum of distances from each leg's
 * achieved/scored reference points respectively for prior task points,
 * and targets or reference points for active and later task points).
 * 
 * @return Distance (m) of planned task
 */
  virtual double scan_distance_planned();

/** 
 * Calculate distance of planned task (sum of distances from aircraft to
 * current target/reference and for later task points from each leg's
 * targets or reference points).
 * 
 * @param ref Location of aircraft
 * 
 * @return Distance (m) remaining in the planned task
 */ 
  virtual double scan_distance_remaining(const GEOPOINT &ref);

/** 
 * Calculate scored distance of achieved part of task.
 * 
 * @param ref Location of aircraft
 * 
 * @return Distance (m) achieved adjusted for scoring
 */
  virtual double scan_distance_scored(const GEOPOINT &ref);

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
  virtual double scan_distance_travelled(const GEOPOINT &ref);

/** 
 * Calculate maximum and minimum distances for task, achievable
 * from the current aircraft state (assuming active taskpoint does not retreat). 
 * 
 * @param ref Aircraft location
 * @param full Perform full search (if task state has changed)
 * @param dmin Minimum distance (m) achievable of task
 * @param dmax Maximum distance (m) achievable of task
 */
  virtual void scan_distance_minmax(const GEOPOINT &ref, 
                                    bool full,
                                    double *dmin, double *dmax);

/** 
 * Calculate glide result for remainder of task
 * 
 * @param state_now Aircraft state
 * @param total Glide result accumulated for total remaining task
 * @param leg Glide result for current leg of task
 */
  virtual void glide_solution_remaining(const AIRCRAFT_STATE &state_now, 
                                        GlideResult &total,
                                        GlideResult &leg);

/** 
 * Calculate glide result from start of task to current state
 * 
 * @param state_now Aircraft state
 * @param total Glide result accumulated for total travelled task
 * @param leg Glide result for current leg of task
 */
  virtual void glide_solution_travelled(const AIRCRAFT_STATE &state_now, 
                                        GlideResult &total,
                                        GlideResult &leg);

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
                                      DistanceRemainingStat &total_remaining_effective,
                                      DistanceRemainingStat &leg_remaining_effective,
                                      const double total_t_elapsed,
                                      const double leg_t_elapsed);

protected:

  unsigned activeTaskPoint;
  TaskStats stats;
  TaskAdvance &task_advance;
  GlidePolar glide_polar;
  const TaskEvents &task_events;
  const TaskBehaviour &task_behaviour;

  void update_stats_distances(const GEOPOINT &location,
                              const bool full_update);

private:
  void update_glide_solutions(const AIRCRAFT_STATE &state);
  void update_stats_times(const AIRCRAFT_STATE &);
  void update_stats_speeds(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&);
  void update_stats_glide(const AIRCRAFT_STATE &state);
  double leg_gradient(const AIRCRAFT_STATE &state);

  Filter mc_lpf;
  Filter ce_lpf;

  bool trigger_auto;

public:
#ifdef DO_PRINT
  virtual void print(const AIRCRAFT_STATE&);
#endif

  virtual void Accept(TaskPointVisitor& visitor) const = 0;
  DEFINE_VISITABLE()
};
#endif //ABSTRACTTASK_H
