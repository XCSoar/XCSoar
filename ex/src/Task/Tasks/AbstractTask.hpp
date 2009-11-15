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
    ce_lpf(60.0)
  {};

/** 
 * Retrieves the active task point sequence.
 * 
 * @return Index of active task point sequence
 */
    unsigned getActiveTaskPointIndex();

/** 
 * Accessor for task statistics for this task
 * 
 * @return Task statistics reference
 */
    virtual const TaskStats& get_stats() const {
      return stats;
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
 * @return True if internal state changes
 */
  virtual bool update_sample(const AIRCRAFT_STATE &, 
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
  

  virtual double calc_mc_best(const AIRCRAFT_STATE &state_now);

  virtual double calc_glide_required(const AIRCRAFT_STATE &state_now);

  virtual double calc_cruise_efficiency(const AIRCRAFT_STATE &state_now) {
    return 1.0;
  }
  virtual double calc_min_target(const AIRCRAFT_STATE &state_now, 
                                 const double t_target) {
    return 0.0;
  };


  virtual double scan_total_start_time(const AIRCRAFT_STATE &state_now);
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
 * @param location Aircraft state
 * @param full Perform full search (if task state has changed)
 * @param dmin Minimum distance (m) achievable of task
 * @param dmax Maximum distance (m) achievable of task
 */
  virtual void scan_distance_minmax(const GEOPOINT &location, bool full,
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

  virtual void glide_solution_planned(const AIRCRAFT_STATE &state_now,
                                      GlideResult &total,
                                      GlideResult &leg,
                                      DistanceRemainingStat &total_remaining_effective,
                                      DistanceRemainingStat &leg_remaining_effective,
                                      const double total_t_elapsed,
                                      const double leg_t_elapsed);

  virtual double calc_gradient(const AIRCRAFT_STATE &state_now);

protected:

  unsigned activeTaskPoint;
  TaskStats stats;
  Filter mc_lpf;
  Filter ce_lpf;

  const TaskEvents &task_events;
  TaskAdvance &task_advance;
  GlidePolar glide_polar;
  const TaskBehaviour &task_behaviour;

private:
  void update_glide_solutions(const AIRCRAFT_STATE &state);
  void update_stats_distances(const GEOPOINT &location,
                              const bool full_update);
  void update_stats_times(const AIRCRAFT_STATE &);
  void update_stats_speeds(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&);
  void update_stats_glide(const AIRCRAFT_STATE &state);
  double leg_gradient(const AIRCRAFT_STATE &state);

public:
#ifdef DO_PRINT
  virtual void print(const AIRCRAFT_STATE&);
#endif

  virtual void Accept(TaskPointVisitor& visitor) const = 0;
  DEFINE_VISITABLE()
};
#endif //ABSTRACTTASK_H
