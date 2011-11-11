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

#ifndef ORDEREDTASK_H
#define ORDEREDTASK_H

#include "Navigation/TaskProjection.hpp"
#include "AbstractTask.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include <vector>
#include "GlideSolvers/MacCready.hpp"
#include "PathSolvers/TaskDijkstraMin.hpp"
#include "PathSolvers/TaskDijkstraMax.hpp"
#include "Task/TaskAdvanceSmart.hpp"
#include "Task/TaskBehaviour.hpp"

#include <assert.h>

class OrderedTaskPoint;
class AbstractTaskFactory;
class Waypoints;
class AATPoint;
class FlatBoundingBox;
struct GeoBounds;
struct TaskSummary;

/**
 * A task comprising an ordered sequence of task points, each with
 * observation zones.  A valid OrderedTask has a StartPoint, zero or more
 * IntermediatePoints and a FinishPoint.
 *
 * \todo
 * - better handling of removal of start/finish point
 * - allow for TakeOffPoint and LandingPoint
 * - have a method to check if a potential taskpoint is distinct from its neighbours?
 * - multiple start points
 */
class OrderedTask:
  public AbstractTask
{
public:
  friend class Serialiser;
  friend class TaskDijkstra;
  friend class TaskDijkstraMin;
  friend class TaskDijkstraMax;
  friend class PrintHelper;

  typedef std::vector<OrderedTaskPoint*> OrderedTaskPointVector; /**< Storage type of task points */ 

private:
  OrderedTaskPointVector task_points;
  OrderedTaskPointVector optional_start_points;

  StartPoint *taskpoint_start;
  FinishPoint *taskpoint_finish;

  TaskProjection task_projection;

  GeoPoint m_location_min_last;

  TaskBehaviour::Factory_t factory_mode;
  AbstractTaskFactory* active_factory;
  OrderedTaskBehaviour m_ordered_behaviour;
  TaskAdvanceSmart task_advance;
  TaskDijkstraMin dijkstra_min;
  TaskDijkstraMax dijkstra_max;

public:
  /** 
   * Constructor.
   *
   * \todo
   * - default values in constructor
   * 
   * @param te Task events
   * @param tb Task behaviour
   * @param gp Glide Polar
   * @param do_reserve Whether to reserve dijkstra queue storage
   * 
   * @return Initialised object
   */
  OrderedTask(TaskEvents &te, 
              const TaskBehaviour &tb,
              const GlidePolar &gp,
              const bool do_reserve=false);
  virtual ~OrderedTask();

  virtual void SetTaskBehaviour(const TaskBehaviour &tb);

  /**
   * Accessor for factory system for constructing tasks
   *
   * @return Factory
   */
  gcc_pure
  AbstractTaskFactory& get_factory() const {
    return *active_factory;
  }

  /**
   * Set type of task factory to be used for constructing tasks
   *
   * @param _factory Type of task
   *
   * @return Type of task
   */
  TaskBehaviour::Factory_t set_factory(const TaskBehaviour::Factory_t _factory);

  /** 
   * Return list of factory types
   * 
   * @param all If true, return all types, otherwise only valid transformable ones
   * 
   * @return Vector of factory types
   */
  gcc_pure
  std::vector<TaskBehaviour::Factory_t> get_factory_types(bool all=true) const;

  /** 
   * Reset the task (as if never flown)
   * 
   */
  void reset();

  /** 
   * Clear all points and restore default ordered task behaviour
   * 
   */
  void clear();

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
  OrderedTask* clone(TaskEvents &te, 
                     const TaskBehaviour &tb,
                     const GlidePolar &gp) const;

  /**
   * Copy task into this task
   *
   * @param other OrderedTask to copy
   * @return True if this task changed
   */
  bool commit(const OrderedTask& other);

  /**
   * Retrieves the active task point sequence.
   *
   * @return Active task point
   */
  gcc_pure
  TaskWaypoint* GetActiveTaskPoint() const;

  /**
   * Retrieves the active task point index.
   *
   * @return Index of active task point sequence
   */
  gcc_pure
  unsigned getActiveIndex() const;

  /**
   * Retrieve task point by sequence index
   *
   * @param index Index of task point sequence
   *
   * @return OrderedTaskPoint at index (or NULL if out of range)
   */
  gcc_pure
  const OrderedTaskPoint* getTaskPoint(const unsigned index) const;

  /**
   * Determine whether active task point optionally shifted points to
   * a valid task point.
   *
   * @param index_offset offset (default 0)
   */
  gcc_pure
  bool IsValidTaskPoint(const int index_offset=0) const;

  /**
   * Check if task has a single StartPoint
   *
   * @return True if task has start
   */
  gcc_pure
  bool has_start() const {
    return taskpoint_start != NULL;
  }

  /**
   * Check if task has a single FinishPoint
   *
   * @return True if task has finish
   */
  gcc_pure
  bool has_finish() const {
    return taskpoint_finish != NULL;
  }

  /**
   * Set active task point index
   *
   * @param desired Desired active index of task sequence
   */
  void SetActiveTaskPoint(unsigned desired);

  /**
   * Cycle through optional start points, replacing actual task start point
   * with top item in optional starts.
   */
  void rotateOptionalStarts();

  /**
   * Return number of optional start points
   */
  unsigned optional_starts_size() const;

  /**
   * Insert taskpoint before specified index in task.  May fail if the candidate
   * is the wrong type (e.g. if it is a StartPoint and the task already
   * has one).
   * Ownership is transferred to this object.
   *
   * @param tp Taskpoint to insert
   * @param position Index in task sequence, before which to insert
   *
   * @return True on success
   */
  bool insert(const OrderedTaskPoint &tp, const unsigned position);

  /**
   * Replace taskpoint.
   * May fail if the candidate is the wrong type.
   * Does nothing (but returns true) if replacement is equivalent
   * Ownership is transferred to this object.
   *
   * @param tp Taskpoint to become replacement
   * @param position Index in task sequence of task point to replace
   *
   * @return True on success
   */
  bool replace(const OrderedTaskPoint &tp, const unsigned position);

  /**
   * Replace optional start point.
   * May fail if the candidate is the wrong type.
   * Does nothing (but returns true) if replacement is equivalent
   * Ownership is transferred to this object.
   *
   * @param tp Taskpoint to become replacement
   * @param position Index in task sequence of task point to replace
   *
   * @return True on success
   */
  bool replace_optional_start(const OrderedTaskPoint &tp, const unsigned position);

  /**
   * Append taskpoint to end of task.  May fail if the candidate
   * is the wrong type (e.g. if it is a StartPoint and the task already
   * has one).
   * Ownership is transferred to this object.
   *
   * @param tp Taskpoint to append to task
   *
   * @return True on success
   */
  bool append(const OrderedTaskPoint &tp);

  /**
   * Append optional start point.  May fail if the candidate
   * is the wrong type.
   * Ownership is transferred to this object.
   *
   * @param tp Taskpoint to append to task
   *
   * @return True on success
   */
  bool append_optional_start(const OrderedTaskPoint &tp);

  /**
   * Remove task point at specified position.  Note that
   * currently start/finish points can't be removed.
   *
   * @param position Index in task sequence of task point to remove
   *
   * @return True on success
   */
  bool remove(const unsigned position);

  /**
   * Remove optional start point at specified position.
   *
   * @param position Index in sequence of optional start point to remove
   *
   * @return True on success
   */
  bool remove_optional_start(const unsigned position);

  /**
   * Change the waypoint of an optional start point
   * @param position valid index to optional start point
   * @param waypoint
   * @return true if succeeded
   */
  bool relocate_optional_start(const unsigned position, const Waypoint& waypoint);
  /**
   * Relocate a task point to a new location
   *
   * @param position Index in task sequence of task point to replace
   * @param waypoint Waypoint of replacement
   *
   * @return True on success
   */
  bool relocate(const unsigned position, const Waypoint& waypoint);

  /**
   * Check if task is valid.  Calls task_event methods on failure.
   *
   * @return True if task is valid
   */
  bool check_task() const;

 /**
  * returns pointer to AATPoint accessed via TPIndex if exist
  *
  * @param TPindex index of taskpoint
  *
  * @return pointer to tp if valid, else NULL
  */
 AATPoint* get_AAT_task_point(unsigned TPindex) const;

  
  /**
   * Test if task has finished.  Used to determine whether
   * or not to continue updating stats.
   *
   * @return True if task is finished
   */
  bool task_finished() const;

  /**
   * Test if task has started.  Used to determine whether
   * or not update stats.
   *
   * @param soft If true, allow soft starts
   *
   * @return True if task has started
   */
  virtual bool task_started(bool soft=false) const;

  /**
   * Update internal states when aircraft state advances.
   *
   * @param state_now Aircraft state at this time step
   * @param full_update Force update due to task state change
   *
   * @return True if internal state changes
   */
  bool update_sample(const AircraftState &state_now, 
                     const bool full_update);


  /**
   * Update internal states (non-essential) for housework, or where functions are slow
   * and would cause loss to real-time performance.
   *
   * @param state_now Aircraft state at this time step
   *
   * @return True if internal state changed
   */
  bool UpdateIdle(const AircraftState& state_now);

  /**
   * Return size of task
   *
   * @return Number of task points in task
   */
  unsigned TaskSize() const;

  /**
   * Determine whether the task is full according to the factory in use
   *
   * @return True if task is full
   */
  bool is_max_size() const {
    return TaskSize() == m_ordered_behaviour.max_points;
  }

  /**
   * Accessor for task projection, for use when creating task points
   *
   * @return Task global projection
   */
  gcc_pure
  const TaskProjection&
  get_task_projection() const {
    return task_projection;
  }

  /**
   * Accesses task start state
   *
   * @return State at task start (or null state if not started)
   */
  gcc_pure
  AircraftState get_start_state() const;

  /**
   * Accesses task finish state
   *
   * @return State at task finish (or null state if not finished)
   */
  gcc_pure
  AircraftState get_finish_state() const;

  fixed get_finish_height() const;

  GeoPoint get_task_center(const GeoPoint& fallback_location) const;
  fixed get_task_radius(const GeoPoint& fallback_location) const;

  bool has_targets() const;

  void check_duplicate_waypoints(Waypoints& waypoints);

  /**
   * Update internal geometric state of task points.
   * Typically called after task geometry or observation zones are modified.
   *
   *
   * This also updates planned/nominal distances so clients can use that
   * data during task construction.
   */
  void update_geometry();

  /**
   * Convert a GeoBounds into a flat bounding box projected
   * according to the task projection.
   */
  FlatBoundingBox get_bounding_box(const GeoBounds& bounds) const;

  /**
   * Convert a GeoPoint into a flat bounding box projected
   * according to the task projection.
   */
  FlatBoundingBox get_bounding_box(const GeoPoint& point) const;

  /**
   * Update summary task statistics (progress along path)
   */
  void update_summary(TaskSummary& summary) const;

protected:
  bool is_scored() const;

  /**
   * Retrieve vector of search points to be used in max/min distance
   * scans (by TaskDijkstra).
   *
   * @param tp Index of task point of query
   *
   * @return Vector of search point candidates
   */
  const SearchPointVector& get_tp_search_points(unsigned tp) const;

  /**
   * Set task point's minimum distance value (by TaskDijkstra).
   *
   * @param tp Index of task point to set min
   * @param sol Search point found to be minimum distance
   */
  void set_tp_search_min(unsigned tp, const SearchPoint &sol);

  /**
   * Set task point's maximum distance value (by TaskDijkstra).
   *
   * @param tp Index of task point to set max
   * @param sol Search point found to be maximum distance
   */
  void set_tp_search_max(unsigned tp, const SearchPoint &sol);

  /**
   * Set task point's minimum distance achieved value
   *
   * @param tp Index of task point to set min
   * @param sol Search point found to be minimum distance
   */
  void set_tp_search_achieved(unsigned tp, const SearchPoint &sol);

  /**
   * Scan task for valid start/finish points
   *
   * @return True if start and finish found
   */
  bool scan_start_finish();

  /**
   * Test whether (and how) transitioning into/out of task points should occur, typically
   * according to task_advance mechanism.  This also may call the task_event callbacks.
   *
   * @param state_now Aircraft state at this time step
   * @param state_last Aircraft state at previous time step
   *
   * @return True if transition occurred
   */
  bool check_transitions(const AircraftState &state_now, 
                         const AircraftState &state_last);

  /**
   * Calculate distance of nominal task (sum of distances from each
   * leg's consecutive reference point to reference point for entire task).
   *
   * @return Distance (m) of nominal task
   */
  fixed scan_distance_nominal();

  /**
   * Calculate distance of planned task (sum of distances from each leg's
   * achieved/scored reference points respectively for prior task points,
   * and targets or reference points for active and later task points).
   *
   * @return Distance (m) of planned task
   */
  fixed scan_distance_planned();

  /**
   * Calculate distance of planned task (sum of distances from aircraft to
   * current target/reference and for later task points from each leg's
   * targets or reference points).
   *
   * @param ref Location of aircraft
   *
   * @return Distance (m) remaining in the planned task
   */
  fixed scan_distance_remaining(const GeoPoint &ref);

  /**
   * Calculate scored distance of achieved part of task.
   *
   * @param ref Location of aircraft
   *
   * @return Distance (m) achieved adjusted for scoring
   */
  fixed scan_distance_scored(const GeoPoint &ref);

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
  fixed scan_distance_travelled(const GeoPoint &ref);

  /**
   * Calculate maximum and minimum distances for task, achievable
   * from the current aircraft state (assuming active taskpoint does not retreat).
   *
   * @param ref Aircraft location
   * @param full Perform full search (if task state has changed)
   * @param dmin Minimum distance (m) achievable of task
   * @param dmax Maximum distance (m) achievable of task
   */
  void scan_distance_minmax(const GeoPoint &ref, 
                            bool full,
                            fixed *dmin, fixed *dmax);

  /**
   * Calculate task start time.
   *
   * @param state_now Aircraft state
   *
   * @return Time (s) of start of task
   */
  fixed scan_total_start_time(const AircraftState &state_now);

  /**
   * Calculate leg start time.
   *
   * @param state_now Aircraft state
   *
   * @return Time (s) of start of leg
   */
  fixed scan_leg_start_time(const AircraftState &state_now);


  /**
   * Calculate glide result for remainder of task
   *
   * @param state_now Aircraft state
   * @param polar Glide polar used for calculation
   * @param total Glide result accumulated for total remaining task
   * @param leg Glide result for current leg of task
   */
  void glide_solution_remaining(const AircraftState &state_now, 
                                const GlidePolar &polar,
                                GlideResult &total,
                                GlideResult &leg);

  /**
   * Calculate glide result from start of task to current state
   *
   * @param state_now Aircraft state
   * @param total Glide result accumulated for total travelled task
   * @param leg Glide result for current leg of task
   */
  void glide_solution_travelled(const AircraftState &state_now, 
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
  void glide_solution_planned(const AircraftState &state_now, 
                              GlideResult &total,
                              GlideResult &leg,
                              DistanceStat &total_remaining_effective,
                              DistanceStat &leg_remaining_effective,
                              const fixed total_t_elapsed,
                              const fixed leg_t_elapsed);

  /**
   * Calculate/search for best MC, being the highest MC value to produce a
   * pure glide solution for the remainder of the task.
   *
   * @param state_now Aircraft state
   * @param best Best MC value found (m/s)
   *
   * @return true if solution is valid
   */
  bool calc_mc_best(const AircraftState &state_now, fixed& best) const;

  /**
   * Calculate virtual sink rate of aircraft that allows a pure glide solution
   * for the remainder of the task.  Glide is performed according to Mc theory
   * speed with the current glide polar, neglecting effect of virtual sink rate.
   *
   * @param state_now Aircraft state
   *
   * @return Sink rate of aircraft (m/s)
   */
  fixed calc_glide_required(const AircraftState &state_now) const;

  /**
   * Calculate cruise efficiency for the travelled part of the task.
   * This is the ratio of the achieved inter-thermal cruise speed to that
   * predicted by MacCready theory with the current glide polar.
   *
   * @param state_now Aircraft state
   * @param value Output cruise efficiency value (0-)
   *
   * @return True if cruise efficiency is updated
   */
  bool calc_cruise_efficiency(const AircraftState &state_now, fixed& value) const;

  bool calc_effective_mc(const AircraftState &state_now, fixed& value) const;

  /**
   * Optimise target ranges (for adjustable tasks) to produce an estimated
   * time remaining with the current glide polar equal to a target value.
   * This adjusts the target locations for the remainder of the task.
   *
   * @param state_now Aircraft state
   * @param t_target Desired time for remainder of task (s)
   *
   * @return Target range parameter (0-1)
   */
  fixed calc_min_target(const AircraftState &state_now, 
                        const fixed t_target);

  /**
   * Calculate angle from aircraft to remainder of task (minimum of leg
   * heights above turnpoints divided by distance to go for each leg).
   *
   * @param state_now Aircraft state
   *
   * @return Minimum gradient angle of remainder of task
   */
  fixed calc_gradient(const AircraftState &state_now) const;

private:

  fixed scan_distance_min(const GeoPoint &ref, bool full);
  fixed scan_distance_max();

  /**
   * Sets previous/next taskpoint pointers for task point at specified
   * index in sequence.
   *
   * @param position Index of task point
   */
  void set_neighbours(unsigned position);

  /**
   * Erase taskpoint in sequence (for internal use)
   *
   * @param i index of task point in sequence
   */
  void erase(unsigned i);

  /**
   * Erase optional start point (for internal use)
   *
   * @param i index of optional start point in sequence
   */
  void erase_optional_start(unsigned i);

  void update_start_transition(const AircraftState &state, OrderedTaskPoint& start);

  gcc_pure
  bool distance_is_significant(const GeoPoint &location,
                               const GeoPoint &location_last) const;

  bool allow_incremental_boundary_stats(const AircraftState &state) const;

  bool check_transition_point(OrderedTaskPoint& point,
                              const AircraftState &state_now, 
                              const AircraftState &state_last,
                              const FlatBoundingBox& bb_now,
                              const FlatBoundingBox& bb_last,
                              bool &transition_enter,
                              bool &transition_exit,
                              bool &last_started,
                              const bool is_start);

  bool check_transition_optional_start(const AircraftState &state_now, 
                                       const AircraftState &state_last,
                                       const FlatBoundingBox& bb_now,
                                       const FlatBoundingBox& bb_last,
                                       bool &transition_enter,
                                       bool &transition_exit,
                                       bool &last_started);

  /**
   * @param waypoints Active waypoint database
   * @param points Vector of points to confirm in active waypoint database
   * @param is_task True if task point.  False if optional start point
   */
  void check_duplicate_waypoints(Waypoints& waypoints,
                                 OrderedTaskPointVector& points,
                                 const bool is_task);

  void select_optional_start(unsigned pos);

public:
  /**
   * Retrieve TaskAdvance mechanism
   *
   * @return Reference to TaskAdvance used by this task
   */
  const TaskAdvance &get_task_advance() const {
    return task_advance;
  }

  /** 
   * Retrieve TaskAdvance mechanism
   * 
   * @return Reference to TaskAdvance used by this task
   */
  TaskAdvance& get_task_advance() {
    return task_advance;
  }

  /** 
   * Retrieve the factory type used by this task
   * 
   * @return Factory type
   */
  TaskBehaviour::Factory_t get_factory_type() const {
    return factory_mode;
  }

  /** 
   * Retrieve (const) the OrderedTaskBehaviour used by this task
   * 
   * @return Read-only OrderedTaskBehaviour
   */
  const OrderedTaskBehaviour& get_ordered_task_behaviour() const {
    return m_ordered_behaviour;
  }

  /** 
   * Retrieve the OrderedTaskBehaviour used by this task
   * 
   * @return Reference to OrderedTaskBehaviour
   */
  OrderedTaskBehaviour& get_ordered_task_behaviour() {
    return m_ordered_behaviour;
  }

  /** 
   * Copy OrderedTaskBehaviour to this task
   * 
   * @param ob Value to set
   */
  void set_ordered_task_behaviour(const OrderedTaskBehaviour& ob);

  /** 
   * Retrieve task point by index
   * 
   * @param index index position of task point
   * @return Task point or NULL if not found
   */
  gcc_pure
  OrderedTaskPoint* get_tp(const unsigned index);

  gcc_pure
  OrderedTaskPoint &GetPoint(const unsigned i) {
    assert(i < task_points.size());
    assert(task_points[i] != NULL);

    return *task_points[i];
  }

  gcc_pure
  const OrderedTaskPoint &GetPoint(const unsigned i) const {
    assert(i < task_points.size());
    assert(task_points[i] != NULL);

    return *task_points[i];
  }

  /**
   * @return number of optional start poitns
   */
  gcc_pure
  unsigned optional_start_points_size() const {
    return optional_start_points.size();
  }

  /**
   * returns optional start point
   *
   * @param pos optional start point index
   * @return NULL if index out of range, else optional start point
   */
  gcc_pure
  const OrderedTaskPoint * get_optional_start(unsigned pos) const;



  /**
   * Accept a (const) task point visitor; makes the visitor visit
   * all TaskPoint in the task
   *
   * @param visitor Visitor to accept
   * @param reverse Visit task points in reverse order
   */
  void tp_CAccept(TaskPointConstVisitor& visitor, const bool reverse=false) const;

  /**
   * Accept a (const) task point visitor; makes the visitor visit
   * all optional start points in the task
   *
   * @param visitor Visitor to accept
   * @param reverse Visit task points in reverse order
   */
  void sp_CAccept(TaskPointConstVisitor& visitor, const bool reverse=false) const;

};

#endif //ORDEREDTASK_H
