/*
  Copyright_License {

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

#ifndef ORDEREDTASKPOINT_HPP
#define ORDEREDTASKPOINT_HPP

#include "Task/Points/TaskLeg.hpp"
#include "Task/Points/TaskWaypoint.hpp"
#include "Task/Points/ScoredTaskPoint.hpp"
#include "Task/ObservationZones/ObservationZoneClient.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"

#include <memory>

struct TaskBehaviour;
struct OrderedTaskSettings;
class FlatProjection;
class GeoBounds;

/**
 * Abstract compound specialisation of TaskLeg and ScoredTaskPoint,
 * for task points which are organised in an ordered sequence.  This
 * class manages the concept of an active task point, and therefore in
 * a task, one OrderedTaskPoint will be marked as active, and the
 * others marked either before or after active.
 *
 * The OrderedTaskPoint tracks previous and next OrderedTaskPoints.
 */
class OrderedTaskPoint
  :public TaskLeg,
   public TaskWaypoint,
   public ScoredTaskPoint,
   public ObservationZoneClient
{
public:
  /**
   * States each task point can be in
   * (with respect to which OrderedTaskPoint is active/selected).
   */
  enum ActiveState {
    /** This taskpoint is before the active one */
    BEFORE_ACTIVE,
    /** This taskpoint is currently the active one */
    CURRENT_ACTIVE,
    /** This taskpoint is after the active one */
    AFTER_ACTIVE
  };

private:
  /** ActiveState determined from ScanActive() */
  ActiveState active_state;

  OrderedTaskPoint* tp_next;
  OrderedTaskPoint* tp_previous;
  FlatBoundingBox flat_bb;

public:
  /**
   * Constructor.
   * Ownership of oz is transferred to this object
   *
   * @param _oz Observation zone for this task point
   * @param wp Waypoint associated with this task point
   * @param tb Task Behaviour defining options (esp safety heights)
   * @param b_scored Whether distance within OZ is scored
   *
   * @return Partially initialised object
   */
  OrderedTaskPoint(TaskPointType _type,
                   std::unique_ptr<ObservationZonePoint> &&_oz,
                   WaypointPtr &&wp,
                   const bool b_scored) noexcept;

  virtual ~OrderedTaskPoint() noexcept = default;

  /* choose TaskPoint's implementation, not SampledTaskPoint's */
  using TaskPoint::GetLocation;

  /**
   * Create a clone of the task point.
   * Caller is responsible for destruction.
   *
   * @param task_behaviour Task behaviour of clone
   * @param ordered_task_settings Ordered task behaviour of clone
   * @param waypoint Waypoint to shift to (or NULL)
   */
  std::unique_ptr<OrderedTaskPoint> Clone(const TaskBehaviour &task_behaviour,
                                          const OrderedTaskSettings &ordered_task_settings,
                                          WaypointPtr &&waypoint={}) const noexcept;

  /**
   * Update observation zone geometry (or other internal data) when
   * previous/next turnpoint changes.
   */
  void UpdateGeometry() noexcept;

  /** Is it possible to insert a task point before this one? */
  bool IsPredecessorAllowed() const noexcept {
    return GetType() != TaskPointType::START;
  }

  /** Is it possible to insert a task point after this one? */
  bool IsSuccessorAllowed() const noexcept {
    return GetType() != TaskPointType::FINISH;
  }

  virtual void SetTaskBehaviour([[maybe_unused]] const TaskBehaviour &tb) noexcept {}
  virtual void SetOrderedTaskSettings([[maybe_unused]] const OrderedTaskSettings &otb) noexcept {}

  /**
   * Set previous/next task points.
   *
   * @param prev Previous (incoming leg's origin) task point
   * @param next Next (outgoing leg's destination) task point
   */
  virtual void SetNeighbours(OrderedTaskPoint *previous,
                             OrderedTaskPoint *next) noexcept;

  /**
   * Accessor for previous task point
   *
   * @return Previous task point
   */
  const OrderedTaskPoint *GetPrevious() const noexcept {
    return tp_previous;
  }

  OrderedTaskPoint *GetPrevious() noexcept {
    return tp_previous;
  }

  /**
   * Accessor for next task point
   *
   * @return Next task point
   */
  const OrderedTaskPoint *GetNext() const noexcept {
    return tp_next;
  }

  OrderedTaskPoint *GetNext() noexcept {
    return tp_next;
  }

  /**
   * Accessor for activation state of this task point.
   * This is valid only after ScanActive() has been called.
   *
   * @return Activation state of this task point
   */
  ActiveState GetActiveState() const noexcept {
    return active_state;
  }

  /**
   * Are we past this task point?
   */
  bool IsPast() const noexcept {
    return active_state == BEFORE_ACTIVE;
  }

  /**
   * Is this the current task point?
   */
  bool IsCurrent() const noexcept {
    return active_state == CURRENT_ACTIVE;
  }

  /**
   * Do we expect to reach this task point in the future?
   */
  bool IsFuture() const noexcept {
    return active_state == AFTER_ACTIVE;
  }

  /**
   * Scan forward through successors to set the activity
   * state of all connected task points.  Should only be
   * called on the known first task point in the list.
   *
   * @param atp The current active task point
   *
   * @return True if the active task point is found
   */
  bool ScanActive(const OrderedTaskPoint &atp) noexcept;

  /**
   * Test whether a taskpoint is equivalent to this one
   *
   * For this abstract orderedtaskpoint, only compare OZ and WP
   *
   * @param other Taskpoint to compare to
   *
   * @return True if same WP, type and OZ
   */
  [[gnu::pure]]
  virtual bool Equals(const OrderedTaskPoint &other) const noexcept;

  /**
   * Update a GeoBounds to include this taskpoint and observation
   * zone.
   *
   * @param bounds GeoBounds to update
   */
  void ScanBounds(GeoBounds &bounds) const noexcept;

  void UpdateOZ(const FlatProjection &projection) noexcept;

  /**
   * Update the bounding box in flat projected coordinates
   */
  void UpdateBoundingBox(const FlatProjection &projection) noexcept;

  /**
   * Test whether a boundingbox overlaps with this oz
   */
  [[gnu::pure]]
  bool BoundingBoxOverlaps(const FlatBoundingBox &bb) const noexcept;

  [[gnu::pure]]
  const SearchPointVector &GetSearchPoints() const noexcept;

  [[gnu::pure]]
  bool CheckEnterTransitionMat(const AircraftState &ref_now,
                               const AircraftState &ref_last) const noexcept {
    return CheckEnterTransition(ref_now, ref_last);
  }

  [[gnu::pure]]
  virtual bool IsInSector(const AircraftState &ref) const noexcept;

  /**
   * Check if aircraft is within observation zone if near, and if so,
   * update the interior sample polygon.
   *
   * @param state Aircraft state
   * @param task_events Callback class for feedback
   *
   * @return True if internal state changed
   */
  virtual bool UpdateSampleNear(const AircraftState &state,
                                const FlatProjection &projection) noexcept;

  /**
   * Perform updates to samples as required if known to be far from the OZ
   *
   * @param state Aircraft state
   * @param task_events Callback class for feedback
   *
   * @return True if internal state changed
   */
  virtual bool UpdateSampleFar([[maybe_unused]] const AircraftState &state,
                               [[maybe_unused]] const FlatProjection &projection) noexcept {
    return false;
  }

protected:
  /**
   * Calculate distance from previous remaining/planned location to a point,
   * and from that point to the next remaining/planned location.
   * Use of this function facilitates speed-up over simply calculating
   * both distances and adding them.
   *
   * @param ref Reference location
   *
   * @return Distance (m)
   */
  [[gnu::pure]]
  double DoubleLegDistance(const GeoPoint &ref) const noexcept;

public:
  /* virtual methods from class TaskPoint */
  const GeoPoint &GetLocationRemaining() const noexcept override {
    return ScoredTaskPoint::GetLocationRemaining();
  }
  GeoVector GetVectorRemaining(const GeoPoint &) const noexcept override {
    return TaskLeg::GetVectorRemaining();
  }
  GeoVector GetNextLegVector() const noexcept override;

protected:
  /* virtual methods from class ScoredTaskPoint */
  bool CheckEnterTransition(const AircraftState &ref_now,
                            const AircraftState &ref_last) const noexcept override;

  bool CheckExitTransition(const AircraftState &ref_now,
                           const AircraftState &ref_last) const noexcept override {
    return CheckEnterTransition(ref_last, ref_now);
  }
};

#endif
