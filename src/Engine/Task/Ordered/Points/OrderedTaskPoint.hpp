/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Task/Points/ScoredTaskPoint.hpp"
#include "Task/ObservationZones/ObservationZoneClient.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"
#include "Compiler.h"

struct OrderedTaskBehaviour;

/**
 *  Abstract compound specialisation of TaskLeg and ScoredTaskPoint,
 *  for task points which are organised in an ordered sequence.  This
 *  class manages the concept of an active task point, and therefore
 *  in a task, one OrderedTaskPoint will be marked as active, and the
 *  others marked either before or after active.
 *
 *  The OrderedTaskPoint tracks previous and next OrderedTaskPoints. 
 */
class OrderedTaskPoint : 
  public TaskLeg,
  public ScoredTaskPoint,
  public ObservationZoneClient
{
public:
  /**
   * States each task point can be in
   * (with respect to which OrderedTaskPoint is active/selected).
   */
  enum ActiveState {
    /** Active task point was not found, ERROR! */
    NOTFOUND_ACTIVE = 0,
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
  OrderedTaskPoint(TaskPointType _type, ObservationZonePoint *_oz,
                   const Waypoint &wp,
                   const bool b_scored);

  virtual ~OrderedTaskPoint() {}

  /**
   * Create a clone of the task point. 
   * Caller is responsible for destruction.
   *
   * @param task_behaviour Task behaviour of clone
   * @param ordered_task_behaviour Ordered task behaviour of clone
   * @param waypoint Waypoint to shift to (or NULL)
   */
  gcc_malloc
  OrderedTaskPoint *Clone(const TaskBehaviour &task_behaviour,
                          const OrderedTaskBehaviour &ordered_task_behaviour,
                          const Waypoint* waypoint=NULL) const;

  /** 
   * Update observation zone geometry (or other internal data) when
   * previous/next turnpoint changes.
   */
  void UpdateGeometry();

  /** Is it possible to insert a task point before this one? */
  bool IsPredecessorAllowed() const {
    return GetType() != TaskPointType::START;
  }

  /** Is it possible to insert a task point after this one? */
  bool IsSuccessorAllowed() const {
    return GetType() != TaskPointType::FINISH;
  }

  virtual void SetOrderedTaskBehaviour(const OrderedTaskBehaviour &otb) {}

  /**
   * Set previous/next task points.
   *
   * @param prev Previous (incoming leg's origin) task point
   * @param next Next (outgoing leg's destination) task point
   */
  virtual void SetNeighbours(OrderedTaskPoint *previous,
                             OrderedTaskPoint *next);

  /**
   * Accessor for previous task point
   *
   * @return Previous task point
   */
  const OrderedTaskPoint *GetPrevious() const {
    return tp_previous;
  }

  OrderedTaskPoint *GetPrevious() {
    return tp_previous;
  }

  /**
   * Accessor for next task point
   *
   * @return Next task point
   */
  const OrderedTaskPoint *GetNext() const {
    return tp_next;
  }

  OrderedTaskPoint *GetNext() {
    return tp_next;
  }
  
  /**
   * Accessor for activation state of this task point.
   * This is valid only after ScanActive() has been called.
   *
   * @return Activation state of this task point
   */
  ActiveState GetActiveState() const {
    return active_state;
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
  bool ScanActive(const OrderedTaskPoint &atp);

  /**
   * Test whether a taskpoint is equivalent to this one
   *
   * For this abstract orderedtaskpoint, only compare OZ and WP
   *
   * @param other Taskpoint to compare to
   *
   * @return True if same WP, type and OZ
   */
  virtual bool Equals(const OrderedTaskPoint &other) const;

  /**
   * Update a TaskProjection to include this taskpoint and observation zone.
   *
   * @param task_projection Projection to update
   */
  void ScanProjection(TaskProjection &task_projection) const;

  /**
   * Update the bounding box in flat projected coordinates
   */
  void UpdateBoundingBox(const TaskProjection &task_projection);

  /**
   * Test whether a boundingbox overlaps with this oz
   */
  gcc_pure
  bool BoundingBoxOverlaps(const FlatBoundingBox &bb) const;

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
  gcc_pure
  fixed DoubleLegDistance(const GeoPoint &ref) const;

public:
  /* virtual methods from class TaskPoint */
  virtual GeoVector GetVectorRemaining(const GeoPoint &reference) const  gcc_override{
    return vector_remaining;
  }
  virtual GeoVector GetVectorPlanned() const gcc_override {
    return vector_planned;
  }
  virtual GeoVector GetVectorTravelled() const gcc_override {
    return vector_travelled;
  }
  virtual GeoVector GetNextLegVector() const gcc_override;

  /* virtual methods from class SampledTaskPoint */
  virtual void UpdateOZ(const TaskProjection &projection) gcc_override;

private:
  /* virtual methods from class SampledTaskPoint */
  virtual bool SearchNominalIfUnsampled() const gcc_override;
  virtual bool SearchBoundaryPoints() const gcc_override;

public:
  /* virtual methods from class SampledTaskPoint */
  virtual bool IsInSector(const AircraftState &ref) const gcc_override;
  virtual OZBoundary GetBoundary() const gcc_override;

protected:
  /* virtual methods from class ScoredTaskPoint */
  virtual bool CheckEnterTransition(const AircraftState &ref_now,
                                    const AircraftState &ref_last) const gcc_override;

  virtual bool CheckExitTransition(const AircraftState &ref_now,
                                   const AircraftState &ref_last) const  gcc_override{
    return CheckEnterTransition(ref_last, ref_now);
  }
};

#endif
