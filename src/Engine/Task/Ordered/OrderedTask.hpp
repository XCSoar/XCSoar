/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Geo/Flat/TaskProjection.hpp"
#include "Task/AbstractTask.hpp"
#include "SmartTaskAdvance.hpp"
#include "Waypoint/Ptr.hpp"
#include "Util/DereferenceIterator.hpp"
#include "Util/StaticString.hxx"

#include <assert.h>
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
  typedef std::vector<OrderedTaskPoint*> OrderedTaskPointVector;

  typedef DereferenceContainerAdapter<const OrderedTaskPointVector,
                                      const OrderedTaskPoint> ConstTaskPointList;

private:
  OrderedTaskPointVector task_points;
  OrderedTaskPointVector optional_start_points;

  StartPoint *taskpoint_start;
  FinishPoint *taskpoint_finish;

  TaskProjection task_projection;

  GeoPoint last_min_location;

  TaskFactoryType factory_mode;
  AbstractTaskFactory* active_factory;
  OrderedTaskSettings ordered_settings;
  SmartTaskAdvance task_advance;
  TaskDijkstraMin *dijkstra_min;
  TaskDijkstraMax *dijkstra_max;

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
  explicit OrderedTask(const TaskBehaviour &tb);
  ~OrderedTask();

  /**
   * Accessor for factory system for constructing tasks
   *
   * @return Factory
   */
  gcc_pure
  AbstractTaskFactory& GetFactory() const {
    return *active_factory;
  }

  gcc_pure
  const TaskFactoryConstraints &GetFactoryConstraints() const;

  /**
   * Set type of task factory to be used for constructing tasks
   *
   * @param _factory Type of task
   */
  void SetFactory(const TaskFactoryType _factory);

  /**
   * Return list of factory types
   *
   * @param all If true, return all types, otherwise only valid transformable ones
   *
   * @return Vector of factory types
   */
  gcc_pure
  std::vector<TaskFactoryType> GetFactoryTypes(bool all = true) const;

  void SetTaskBehaviour(const TaskBehaviour &tb);

  /**
   * Removes all task points.
   */
  void RemoveAllPoints();

  /**
   * Clear all points and restore default ordered task behaviour
   * for the active factory
   */
  void Clear();

  /**
   * Create a clone of the task.
   * Caller is responsible for destruction.
   *
   * @param te Task events
   * @param tb Task behaviour
   *
   * @return Initialised object
   */
  gcc_malloc
  OrderedTask *Clone(const TaskBehaviour &tb) const;

  /**
   * Copy task into this task
   *
   * @param other OrderedTask to copy
   * @param waypoints.  const reference to the waypoint file
   * @return True if this task changed
   */
  bool Commit(const OrderedTask& other);

  /**
   * Retrieves the active task point index.
   *
   * @return Index of active task point sequence
   */
  gcc_pure
  unsigned GetActiveIndex() const {
    return active_task_point;
  }

  /**
   * Retrieve task point by sequence index
   *
   * @param index Index of task point sequence
   *
   * @return OrderedTaskPoint at index
   */
  gcc_pure
  const OrderedTaskPoint &GetTaskPoint(const unsigned index) const {
    assert(index < task_points.size());

    return *task_points[index];
  }

  /**
   * Check if task has a single StartPoint
   *
   * @return True if task has start
   */
  gcc_pure
  bool HasStart() const {
    return taskpoint_start != nullptr;
  }

  /**
   * Check if task has a single FinishPoint
   *
   * @return True if task has finish
   */
  gcc_pure
  bool HasFinish() const {
    return taskpoint_finish != nullptr;
  }

  /**
   * Cycle through optional start points, replacing actual task start point
   * with top item in optional starts.
   */
  void RotateOptionalStarts();

  /**
   * Returns true if there are optional start points.
   */
  gcc_pure
  bool HasOptionalStarts() const {
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
  bool Insert(const OrderedTaskPoint &tp, const unsigned position);

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
  bool Replace(const OrderedTaskPoint &tp, const unsigned position);

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
  bool ReplaceOptionalStart(const OrderedTaskPoint &tp, const unsigned position);

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
  bool Append(const OrderedTaskPoint &tp);

  /**
   * Append optional start point.  May fail if the candidate
   * is the wrong type.
   * Ownership is transferred to this object.
   *
   * @param tp Taskpoint to append to task
   *
   * @return True on success
   */
  bool AppendOptionalStart(const OrderedTaskPoint &tp);

  /**
   * Remove task point at specified position.  Note that
   * currently start/finish points can't be removed.
   *
   * @param position Index in task sequence of task point to remove
   *
   * @return True on success
   */
  bool Remove(const unsigned position);

  /**
   * Remove optional start point at specified position.
   *
   * @param position Index in sequence of optional start point to remove
   *
   * @return True on success
   */
  bool RemoveOptionalStart(const unsigned position);

  /**
   * Change the waypoint of an optional start point
   * @param position valid index to optional start point
   * @param waypoint
   * @return true if succeeded
   */
  bool RelocateOptionalStart(const unsigned position, WaypointPtr &&waypoint);

  /**
   * Relocate a task point to a new location
   *
   * @param position Index in task sequence of task point to replace
   * @param waypoint Waypoint of replacement
   *
   * @return True on success
   */
  bool Relocate(const unsigned position, WaypointPtr &&waypoint);

 /**
  * returns pointer to AATPoint accessed via TPIndex if exist
  *
  * @param TPindex index of taskpoint
  *
  * @return pointer to tp if valid, else nullptr
  */
 AATPoint* GetAATTaskPoint(unsigned index) const;

  /**
   * Check whether the task point with the specified index exists.
   */
  gcc_pure
  bool IsValidIndex(unsigned i) const {
    return i < task_points.size();
  }

  /**
   * Determine whether the task is full according to the factory in use
   *
   * @return True if task is full
   */
  gcc_pure
  bool IsFull() const;

  /**
   * Accessor for task projection, for use when creating task points
   *
   * @return Task global projection
   */
  gcc_pure
  const TaskProjection&
  GetTaskProjection() const {
    return task_projection;
  }

  void CheckDuplicateWaypoints(Waypoints& waypoints);

  /**
   * Update TaskStats::{task_valid, has_targets, is_mat, has_optional_starts}.
   */
  void UpdateStatsGeometry();

  /**
   * Update internal geometric state of task points.
   * Typically called after task geometry or observation zones are modified.
   *
   *
   * This also updates planned/nominal distances so clients can use that
   * data during task construction.
   */
  void UpdateGeometry();

  /**
   * Update summary task statistics (progress along path)
   */
  void UpdateSummary(TaskSummary &summary) const;

public:
  /**
   * Retrieve vector of search points to be used in max/min distance
   * scans (by TaskDijkstra).
   *
   * @param tp Index of task point of query
   *
   * @return Vector of search point candidates
   */
  const SearchPointVector &GetPointSearchPoints(unsigned tp) const;

protected:
  /**
   * Set task point's minimum distance value (by TaskDijkstra).
   *
   * @param tp Index of task point to set min
   * @param sol Search point found to be minimum distance
   */
  void SetPointSearchMin(unsigned tp, const SearchPoint &sol);

  /**
   * Set task point's maximum distance value (by TaskDijkstra).
   *
   * @param tp Index of task point to set max
   * @param sol Search point found to be maximum distance
   */
  void SetPointSearchMax(unsigned tp, const SearchPoint &sol);

  /**
   * Set task point's minimum distance achieved value
   *
   * @param tp Index of task point to set min
   * @param sol Search point found to be minimum distance
   */
  void set_tp_search_achieved(unsigned tp, const SearchPoint &sol);

public:
  /**
   * Scan task for valid start/finish points
   *
   * @return True if start and finish found
   */
  bool ScanStartFinish();

private:

  /**
   * @return true if a solution was found (and applied)
   */
  bool RunDijsktraMin(const GeoPoint &location);


  double ScanDistanceMin(const GeoPoint &ref, bool full);

  /**
   * @return true if a solution was found (and applied)
   */
  bool RunDijsktraMax();

  double ScanDistanceMax();

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
                       const double t_target);

  /**
   * Sets previous/next taskpoint pointers for task point at specified
   * index in sequence.
   *
   * @param position Index of task point
   */
  void SetNeighbours(unsigned position);

  /**
   * Erase taskpoint in sequence (for internal use)
   *
   * @param i index of task point in sequence
   */
  void ErasePoint(unsigned i);

  /**
   * Erase optional start point (for internal use)
   *
   * @param i index of optional start point in sequence
   */
  void EraseOptionalStartPoint(unsigned i);

  void UpdateStartTransition(const AircraftState &state,
                             OrderedTaskPoint &start);

  gcc_pure
  bool DistanceIsSignificant(const GeoPoint &location,
                             const GeoPoint &location_last) const;

  gcc_pure
  bool AllowIncrementalBoundaryStats(const AircraftState &state) const;

  bool CheckTransitionPoint(OrderedTaskPoint &point,
                            const AircraftState &state_now,
                            const AircraftState &state_last,
                            const FlatBoundingBox &bb_now,
                            const FlatBoundingBox &bb_last,
                            bool &transition_enter, bool &transition_exit,
                            bool &last_started,
                            const bool is_start);

  bool CheckTransitionOptionalStart(const AircraftState &state_now,
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
  void CheckDuplicateWaypoints(Waypoints& waypoints,
                               OrderedTaskPointVector& points,
                               const bool is_task);

  void SelectOptionalStart(unsigned pos);

public:
  /**
   * Retrieve TaskAdvance mechanism
   *
   * @return Reference to TaskAdvance used by this task
   */
  const TaskAdvance &GetTaskAdvance() const {
    return task_advance;
  }

  /**
   * Retrieve TaskAdvance mechanism
   *
   * @return Reference to TaskAdvance used by this task
   */
  TaskAdvance &SetTaskAdvance() {
    return task_advance;
  }

  /**
   * Retrieve the factory type used by this task
   *
   * @return Factory type
   */
  TaskFactoryType GetFactoryType() const {
    return factory_mode;
  }

  /**
   * Retrieve (const) the #OrderedTaskSettings used by this task
   *
   * @return Read-only #OrderedTaskSettings
   */
  const OrderedTaskSettings &GetOrderedTaskSettings() const {
    return ordered_settings;
  }

  /**
   * Copy #OrderedTaskSettings to this task
   *
   * @param ob Value to set
   */
  void SetOrderedTaskSettings(const OrderedTaskSettings &ob);

protected:
  /**
   * Propagate a change to the #OrderedTaskSettings to all interested
   * child objects.
   */
  void PropagateOrderedTaskSettings();

public:
  ConstTaskPointList GetPoints() const {
    return task_points;
  }

  gcc_pure
  OrderedTaskPoint &GetPoint(const unsigned i) {
    assert(i < task_points.size());
    assert(task_points[i] != nullptr);

    return *task_points[i];
  }

  gcc_pure
  const OrderedTaskPoint &GetPoint(const unsigned i) const {
    assert(i < task_points.size());
    assert(task_points[i] != nullptr);

    return *task_points[i];
  }

  ConstTaskPointList GetOptionalStartPoints() const {
    return optional_start_points;
  }

  /**
   * @return number of optional start poitns
   */
  gcc_pure
  unsigned GetOptionalStartPointCount() const {
    return optional_start_points.size();
  }

  /**
   * returns optional start point
   *
   * @param pos optional start point index
   * @return nullptr if index out of range, else optional start point
   */
  gcc_pure
  const OrderedTaskPoint &GetOptionalStartPoint(unsigned i) const {
    assert(i < optional_start_points.size());

    return *optional_start_points[i];
  }

  /** Determines whether the task has adjustable targets */
  gcc_pure
  bool HasTargets() const;

  /**
   * Find location of center of task (for rendering purposes)
   *
   * @return Location of center of task or GeoPoint::Invalid()
   */
  gcc_pure
  GeoPoint GetTaskCenter() const;

  /**
   * Find approximate radius of task from center to edge (for rendering purposes)
   *
   * @return Radius (m) from center to edge of task
   */
  gcc_pure
  double GetTaskRadius() const;

  /**
   * returns the index of the highest intermediate TP that has been entered.
   * if none have been entered, returns zero
   * If start has exited, returns zero
   * Does not consider whether Finish has been achieved
   * @return index of last intermediate point achieved or 0 if none
   */
  unsigned GetLastIntermediateAchieved() const;

  gcc_pure
  const StaticString<64> &GetName() const {
    return name;
  }

  void SetName(const StaticString<64> &name_) {
    name = name_;
  }

  void ClearName() {
    name.clear();
  }

public:
  /* virtual methods from class TaskInterface */
  unsigned TaskSize() const override {
    return task_points.size();
  }

  void SetActiveTaskPoint(unsigned desired) override;
  TaskWaypoint *GetActiveTaskPoint() const override;
  bool IsValidTaskPoint(const int index_offset=0) const override;
  bool UpdateIdle(const AircraftState& state_now,
                  const GlidePolar &glide_polar) override;

  /* virtual methods from class AbstractTask */
  void Reset() override;
  bool TaskStarted(bool soft=false) const override;
  bool CheckTask() const override;

protected:
  /* virtual methods from class AbstractTask */
  bool UpdateSample(const AircraftState &state_now,
                    const GlidePolar &glide_polar,
                    const bool full_update) override;
  bool CheckTransitions(const AircraftState &state_now,
                        const AircraftState &state_last) override;
  bool CalcBestMC(const AircraftState &state_now,
                  const GlidePolar &glide_polar,
                  double &best) const override;
  double CalcRequiredGlide(const AircraftState &state_now,
                           const GlidePolar &glide_polar) const override;
  bool CalcCruiseEfficiency(const AircraftState &state_now,
                            const GlidePolar &glide_polar,
                            double &value) const override;
  bool CalcEffectiveMC(const AircraftState &state_now,
                       const GlidePolar &glide_polar,
                       double &value) const override;
  double CalcGradient(const AircraftState &state_now) const override;
  double ScanTotalStartTime() override;
  double ScanLegStartTime() override;
  double ScanDistanceNominal() override;
  double ScanDistancePlanned() override;
  double ScanDistanceRemaining(const GeoPoint &ref) override;
  double ScanDistanceScored(const GeoPoint &ref) override;
  double ScanDistanceTravelled(const GeoPoint &ref) override;
  void ScanDistanceMinMax(const GeoPoint &ref, bool full,
                          double *dmin, double *dmax) override;
  void GlideSolutionRemaining(const AircraftState &state_now,
                              const GlidePolar &polar,
                              GlideResult &total, GlideResult &leg) override;
  void GlideSolutionTravelled(const AircraftState &state_now,
                              const GlidePolar &glide_polar,
                              GlideResult &total, GlideResult &leg) override;
  void GlideSolutionPlanned(const AircraftState &state_now,
                            const GlidePolar &glide_polar,
                            GlideResult &total,
                            GlideResult &leg,
                            DistanceStat &total_remaining_effective,
                            DistanceStat &leg_remaining_effective,
                            const GlideResult &solution_remaining_total,
                            const GlideResult &solution_remaining_leg) override;
protected:
  bool IsScored() const override;

public:
  void AcceptTaskPointVisitor(TaskPointConstVisitor &visitor) const override;
};

#endif //ORDEREDTASK_H
