// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/Flat/TaskProjection.hpp"
#include "Task/AbstractTask.hpp"
#include "SmartTaskAdvance.hpp"
#include "Waypoint/Ptr.hpp"
#include "util/DereferenceIterator.hxx"
#include "util/StaticString.hxx"

#include <cassert>
#include <memory>
#include <vector>

class SearchPoint;
class SearchPointVector;
class OrderedTaskPoint;
class StartPoint;
class FinishPoint;
class AbstractTaskFactory;
class TaskDijkstraMin;
class TaskDijkstraMax;
class Waypoints;
class AATPoint;
struct FlatBoundingBox;
class GeoBounds;
struct TaskSummary;
struct TaskFactoryConstraints;

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
class OrderedTask final : public AbstractTask
{
public:
  /** Storage type of task points */
  using OrderedTaskPointVector = std::vector<std::unique_ptr<OrderedTaskPoint>>;

  using ConstTaskPointList =
    DereferenceContainerAdapter<const OrderedTaskPointVector,
                                const OrderedTaskPoint>;

  using TaskPointList =
    DereferenceContainerAdapter<const OrderedTaskPointVector,
                                OrderedTaskPoint>;

private:
  OrderedTaskPointVector task_points;
  OrderedTaskPointVector optional_start_points;

  StartPoint *taskpoint_start = nullptr;
  FinishPoint *taskpoint_finish = nullptr;

  TaskProjection task_projection;

  GeoPoint last_min_location;

  TaskFactoryType factory_mode;
  std::unique_ptr<AbstractTaskFactory> active_factory;
  OrderedTaskSettings ordered_settings;
  SmartTaskAdvance task_advance;
  std::unique_ptr<TaskDijkstraMin> dijkstra_min;
  std::unique_ptr<TaskDijkstraMax> dijkstra_max;
  std::unique_ptr<TaskDijkstraMax> dijkstra_max_total;

  StaticString<64> name;

public:
  /**
   * Constructor.
   *
   * \todo
   * - default values in constructor
   *
   * @param tb Task behaviour
   *
   * @return Initialised object
   */
  explicit OrderedTask(const TaskBehaviour &tb) noexcept;
  ~OrderedTask() noexcept;

  /**
   * Accessor for factory system for constructing tasks
   *
   * @return Factory
   */
  [[gnu::pure]]
  AbstractTaskFactory &GetFactory() const noexcept {
    return *active_factory;
  }

  [[gnu::pure]]
  const TaskFactoryConstraints &GetFactoryConstraints() const noexcept;

  /**
   * Set type of task factory to be used for constructing tasks
   *
   * @param _factory Type of task
   */
  void SetFactory(const TaskFactoryType _factory) noexcept;

  /**
   * Return list of factory types
   *
   * @param all If true, return all types, otherwise only valid transformable ones
   *
   * @return Vector of factory types
   */
  [[gnu::pure]]
  std::vector<TaskFactoryType> GetFactoryTypes(bool all = true) const noexcept;

  void SetTaskBehaviour(const TaskBehaviour &tb) noexcept;

  /**
   * Removes all task points.
   */
  void RemoveAllPoints() noexcept;

  /**
   * Clear all points and restore default ordered task behaviour
   * for the active factory
   */
  void Clear() noexcept;

  /**
   * Create a clone of the task.
   * Caller is responsible for destruction.
   *
   * @param te Task events
   * @param tb Task behaviour
   *
   * @return Initialised object
   */
  std::unique_ptr<OrderedTask> Clone(const TaskBehaviour &tb) const noexcept;

  /**
   * Copy task into this task
   *
   * @param other OrderedTask to copy
   * @param waypoints.  const reference to the waypoint file
   * @return True if this task changed
   */
  bool Commit(const OrderedTask& other) noexcept;

  /**
   * Retrieves the active task point index.
   *
   * @return Index of active task point sequence
   */
  [[gnu::pure]]
  unsigned GetActiveIndex() const noexcept {
    return active_task_point;
  }

  /**
   * Retrieve task point by sequence index
   *
   * @param index Index of task point sequence
   *
   * @return OrderedTaskPoint at index
   */
  [[gnu::pure]]
  const OrderedTaskPoint &GetTaskPoint(const unsigned index) const noexcept {
    assert(index < task_points.size());

    return *task_points[index];
  }

  /**
   * Check if task has a single StartPoint
   *
   * @return True if task has start
   */
  [[gnu::pure]]
  bool HasStart() const noexcept {
    return taskpoint_start != nullptr;
  }

  /**
   * Check if task has a single FinishPoint
   *
   * @return True if task has finish
   */
  [[gnu::pure]]
  bool HasFinish() const noexcept {
    return taskpoint_finish != nullptr;
  }

  /**
   * Cycle through optional start points, replacing actual task start point
   * with top item in optional starts.
   */
  void RotateOptionalStarts() noexcept;

  /**
   * Returns true if there are optional start points.
   */
  [[gnu::pure]]
  bool HasOptionalStarts() const noexcept {
    return !optional_start_points.empty();
  }

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
  bool Insert(const OrderedTaskPoint &tp, unsigned position) noexcept;

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
  bool Replace(const OrderedTaskPoint &tp, unsigned position) noexcept;

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
  bool ReplaceOptionalStart(const OrderedTaskPoint &tp, unsigned position) noexcept;

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
  bool Append(const OrderedTaskPoint &tp) noexcept;

  /**
   * Append optional start point.  May fail if the candidate
   * is the wrong type.
   * Ownership is transferred to this object.
   *
   * @param tp Taskpoint to append to task
   *
   * @return True on success
   */
  bool AppendOptionalStart(const OrderedTaskPoint &tp) noexcept;

  /**
   * Remove task point at specified position.  Note that
   * currently start/finish points can't be removed.
   *
   * @param position Index in task sequence of task point to remove
   *
   * @return True on success
   */
  bool Remove(unsigned position) noexcept;

  /**
   * Remove optional start point at specified position.
   *
   * @param position Index in sequence of optional start point to remove
   *
   * @return True on success
   */
  bool RemoveOptionalStart(unsigned position) noexcept;

  /**
   * Change the waypoint of an optional start point
   * @param position valid index to optional start point
   * @param waypoint
   * @return true if succeeded
   */
  bool RelocateOptionalStart(unsigned position, WaypointPtr &&waypoint) noexcept;

  /**
   * Relocate a task point to a new location
   *
   * @param position Index in task sequence of task point to replace
   * @param waypoint Waypoint of replacement
   *
   * @return True on success
   */
  bool Relocate(unsigned position, WaypointPtr &&waypoint) noexcept;

 /**
  * returns pointer to AATPoint accessed via TPIndex if exist
  *
  * @param TPindex index of taskpoint
  *
  * @return pointer to tp if valid, else nullptr
  */
 AATPoint* GetAATTaskPoint(unsigned index) const noexcept;

  /**
   * Check whether the task point with the specified index exists.
   */
  [[gnu::pure]]
  bool IsValidIndex(unsigned i) const noexcept {
    return i < task_points.size();
  }

  /**
   * Determine whether the task is full according to the factory in use
   *
   * @return True if task is full
   */
  [[gnu::pure]]
  bool IsFull() const noexcept;

  /**
   * Accessor for task projection, for use when creating task points
   *
   * This object is only valid if the task is not empty (see IsEmpty()).
   *
   * @return Task global projection
   */
  [[gnu::pure]]
  const TaskProjection &GetTaskProjection() const noexcept {
    assert(!IsEmpty());

    return task_projection;
  }

  void CheckDuplicateWaypoints(Waypoints &waypoints) noexcept;

  /**
   * Update TaskStats::{task_valid, has_targets, is_mat, has_optional_starts}.
   */
  void UpdateStatsGeometry() noexcept;

  /**
   * Update internal geometric state of task points.
   * Typically called after task geometry or observation zones are modified.
   *
   *
   * This also updates planned/nominal distances so clients can use that
   * data during task construction.
   */
  void UpdateGeometry() noexcept;

  /**
   * Update summary task statistics (progress along path)
   */
  void UpdateSummary(TaskSummary &summary) const noexcept;

public:
  /**
   * Retrieve vector of search points to be used in max/min distance
   * scans (by TaskDijkstra).
   *
   * @param tp Index of task point of query
   *
   * @return Vector of search point candidates
   */
  const SearchPointVector &GetPointSearchPoints(unsigned tp) const noexcept;

protected:
  /**
   * Set task point's minimum distance value (by TaskDijkstra).
   *
   * @param tp Index of task point to set min
   * @param sol Search point found to be minimum distance
   */
  void SetPointSearchMin(unsigned tp, const SearchPoint &sol) noexcept;

  /**
   * Set task point's maximum flyable distance value (by TaskDijkstra).
   *
   * @param tp Index of task point to set max
   * @param sol Search point found to be maximum distance
   */
  void SetPointSearchMax(unsigned tp, const SearchPoint &sol) noexcept;

/**
   * Set task point's total maximum distance point, irrespective of 
   * currently flown track (by TaskDijkstra).
   *
   * @param tp Index of task point to set max
   * @param sol Search point found to be maximum distance
   */
  void SetPointSearchMaxTotal(unsigned tp, const SearchPoint &sol) noexcept;

  /**
   * Set task point's minimum distance achieved value
   *
   * @param tp Index of task point to set min
   * @param sol Search point found to be minimum distance
   */
  void set_tp_search_achieved(unsigned tp, const SearchPoint &sol) noexcept;

public:
  /**
   * Scan task for valid start/finish points
   *
   * @return True if start and finish found
   */
  bool ScanStartFinish() noexcept;

private:

  /**
   * @return true if a solution was found (and applied)
   */
  bool RunDijsktraMin(const GeoPoint &location) noexcept;

  double ScanDistanceMin(const GeoPoint &ref, bool full) noexcept;

  /**
   * Search the points that give the maximum distance
   * 
   * @param dijkstra Calculator object to use (its state will be updated)
   * @param results Vector of SearchPoints where the resulting points are returned
   * @param ignoreSampledPoints Run the algorithm only on TP boundaries, ignoring flown path
   * 
   * @return true if a solution was found
   */
  bool RunDijsktraMax(TaskDijkstraMax &dijkstra, 
                      SearchPointVector &results, 
                      bool ignoreSampledPoints) const noexcept;

  /**
   * Update the maximum flyable distance points with the TaskDijkstraMax calcualtor 
   * 
   * @return the maximum distance value
   */
  double ScanDistanceMax() noexcept;

  /**
   * Optimise target ranges (for adjustable tasks) to produce an estimated
   * time remaining with the current glide polar, equal to a target value.
   *
   * @param state_now Aircraft state
   * @param t_target Desired time for remainder of task (s)
   *
   * @return Target range parameter (0-1)
   */
  double CalcMinTarget(const AircraftState &state_now,
                       const GlidePolar &glide_polar,
                       const FloatDuration t_target) noexcept;

  /**
   * Sets previous/next taskpoint pointers for task point at specified
   * index in sequence.
   *
   * @param position Index of task point
   */
  void SetNeighbours(unsigned position) noexcept;

  /**
   * Erase taskpoint in sequence (for internal use)
   *
   * @param i index of task point in sequence
   */
  void ErasePoint(unsigned i) noexcept;

  /**
   * Erase optional start point (for internal use)
   *
   * @param i index of optional start point in sequence
   */
  void EraseOptionalStartPoint(unsigned i) noexcept;

  void UpdateStartTransition(const AircraftState &state,
                             OrderedTaskPoint &start) noexcept;

  [[gnu::pure]]
  bool DistanceIsSignificant(const GeoPoint &location,
                             const GeoPoint &location_last) const noexcept;

  [[gnu::pure]]
  bool AllowIncrementalBoundaryStats(const AircraftState &state) const noexcept;

  bool CheckTransitionPoint(OrderedTaskPoint &point,
                            const AircraftState &state_now,
                            const AircraftState &state_last,
                            const FlatBoundingBox &bb_now,
                            const FlatBoundingBox &bb_last,
                            bool &transition_enter, bool &transition_exit,
                            bool is_start) noexcept;

  bool CheckTransitionOptionalStart(const AircraftState &state_now,
                                    const AircraftState &state_last,
                                    const FlatBoundingBox& bb_now,
                                    const FlatBoundingBox& bb_last,
                                    bool &transition_enter,
                                    bool &transition_exit) noexcept;

  /**
   * @param waypoints Active waypoint database
   * @param points Vector of points to confirm in active waypoint database
   * @param is_task True if task point.  False if optional start point
   */
  void CheckDuplicateWaypoints(Waypoints& waypoints,
                               OrderedTaskPointVector& points,
                               bool is_task) noexcept;

  void SelectOptionalStart(unsigned pos) noexcept;

public:
  /**
   * Retrieve TaskAdvance mechanism
   *
   * @return Reference to TaskAdvance used by this task
   */
  const TaskAdvance &GetTaskAdvance() const noexcept {
    return task_advance;
  }

  /**
   * Retrieve TaskAdvance mechanism
   *
   * @return Reference to TaskAdvance used by this task
   */
  TaskAdvance &SetTaskAdvance() noexcept {
    return task_advance;
  }

  /**
   * Retrieve the factory type used by this task
   *
   * @return Factory type
   */
  TaskFactoryType GetFactoryType() const noexcept {
    return factory_mode;
  }

  /**
   * Retrieve (const) the #OrderedTaskSettings used by this task
   *
   * @return Read-only #OrderedTaskSettings
   */
  const OrderedTaskSettings &GetOrderedTaskSettings() const noexcept {
    return ordered_settings;
  }

  /**
   * Copy #OrderedTaskSettings to this task
   *
   * @param ob Value to set
   */
  void SetOrderedTaskSettings(const OrderedTaskSettings &ob) noexcept;

protected:
  /**
   * Propagate a change to the #OrderedTaskSettings to all interested
   * child objects.
   */
  void PropagateOrderedTaskSettings() noexcept;

public:
  /**
   * Check whether the task is empty.
   *
   * Note that even if the task is non-empty, it does not imply that
   * the task is "valid".
   */
  bool IsEmpty() const noexcept {
    return task_points.empty();
  }

  ConstTaskPointList GetPoints() const noexcept {
    return task_points;
  }

  [[gnu::pure]]
  OrderedTaskPoint &GetPoint(const unsigned i) noexcept {
    assert(i < task_points.size());
    assert(task_points[i] != nullptr);

    return *task_points[i];
  }

  [[gnu::pure]]
  const OrderedTaskPoint &GetPoint(const unsigned i) const noexcept {
    assert(i < task_points.size());
    assert(task_points[i] != nullptr);

    return *task_points[i];
  }

  ConstTaskPointList GetOptionalStartPoints() const noexcept {
    return optional_start_points;
  }

  /**
   * @return number of optional start poitns
   */
  [[gnu::pure]]
  std::size_t GetOptionalStartPointCount() const noexcept {
    return optional_start_points.size();
  }

  /**
   * returns optional start point
   *
   * @param pos optional start point index
   * @return nullptr if index out of range, else optional start point
   */
  [[gnu::pure]]
  const OrderedTaskPoint &GetOptionalStartPoint(unsigned i) const noexcept {
    assert(i < optional_start_points.size());

    return *optional_start_points[i];
  }

  /** Determines whether the task has adjustable targets */
  [[gnu::pure]]
  bool HasTargets() const noexcept;

  /**
   * returns the index of the highest intermediate TP that has been entered.
   * if none have been entered, returns zero
   * If start has exited, returns zero
   * Does not consider whether Finish has been achieved
   * @return index of last intermediate point achieved or 0 if none
   */
  unsigned GetLastIntermediateAchieved() const noexcept;

  [[gnu::pure]]
  const StaticString<64> &GetName() const noexcept {
    return name;
  }

  template<typename T>
  void SetName(T &&_name) noexcept {
    name = std::forward<T>(_name);
  }

  void ClearName() noexcept {
    name.clear();
  }

public:
  /* virtual methods from class TaskInterface */
  unsigned TaskSize() const noexcept override {
    return task_points.size();
  }

  void SetActiveTaskPoint(unsigned desired) noexcept override;
  TaskWaypoint *GetActiveTaskPoint() const noexcept override;
  bool IsValidTaskPoint(const int index_offset=0) const noexcept override;
  bool UpdateIdle(const AircraftState& state_now,
                  const GlidePolar &glide_polar) noexcept override;

  /* virtual methods from class AbstractTask */
  void Reset() noexcept override;
  bool TaskStarted(bool soft=false) const noexcept override;
  TaskValidationErrorSet CheckTask() const noexcept override;

protected:
  /* virtual methods from class AbstractTask */
  bool UpdateSample(const AircraftState &state_now,
                    const GlidePolar &glide_polar,
                    const bool full_update) noexcept override;
  bool CheckTransitions(const AircraftState &state_now,
                        const AircraftState &state_last) noexcept override;
  bool CalcBestMC(const AircraftState &state_now,
                  const GlidePolar &glide_polar,
                  double &best) const noexcept override;
  double CalcRequiredGlide(const AircraftState &state_now,
                           const GlidePolar &glide_polar) const noexcept override;
  bool CalcCruiseEfficiency(const AircraftState &state_now,
                            const GlidePolar &glide_polar,
                            double &value) const noexcept override;
  bool CalcEffectiveMC(const AircraftState &state_now,
                       const GlidePolar &glide_polar,
                       double &value) const noexcept override;
  double CalcGradient(const AircraftState &state_now) const noexcept override;
  TimeStamp ScanTotalStartTime() noexcept override;
  TimeStamp ScanLegStartTime() noexcept override;
  double ScanDistanceNominal() const noexcept override;
  double ScanDistancePlanned() noexcept override;
  double ScanDistanceRemaining(const GeoPoint &ref) noexcept override;
  double ScanDistanceScored(const GeoPoint &ref) noexcept override;
  double ScanDistanceTravelled(const GeoPoint &ref) noexcept override;
  void ScanDistanceMinMax(const GeoPoint &ref, bool full,
                          double *dmin, double *dmax) noexcept override;
  double ScanDistanceMaxTotal() noexcept override;
  void GlideSolutionRemaining(const AircraftState &state_now,
                              const GlidePolar &polar,
                              GlideResult &total, GlideResult &leg) noexcept override;
  void GlideSolutionTravelled(const AircraftState &state_now,
                              const GlidePolar &glide_polar,
                              GlideResult &total, GlideResult &leg) noexcept override;
  void GlideSolutionPlanned(const AircraftState &state_now,
                            const GlidePolar &glide_polar,
                            GlideResult &total,
                            GlideResult &leg,
                            DistanceStat &total_remaining_effective,
                            DistanceStat &leg_remaining_effective,
                            const GlideResult &solution_remaining_total,
                            const GlideResult &solution_remaining_leg) noexcept override;
protected:
  bool IsScored() const noexcept override;

public:
  void AcceptTaskPointVisitor(TaskPointConstVisitor &visitor) const override;
};
