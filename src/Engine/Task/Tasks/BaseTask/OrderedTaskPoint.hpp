/*
  Copyright_License {

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

#ifndef ORDEREDTASKPOINT_HPP
#define ORDEREDTASKPOINT_HPP

#include "TaskLeg.hpp"
#include "ScoredTaskPoint.hpp"
#include "ObservationZoneClient.hpp"
#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Compiler.h"

class OrderedTaskBehaviour;

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
  friend class Serialiser;
  friend class PrintHelper;

public:
  /**
   * States each task point can be in (with respect to which OrderedTaskPoint is
   * active/selected).
   */
  enum ActiveState_t {
    NOTFOUND_ACTIVE = 0,        /**< Active task point was not found, ERROR! */
    BEFORE_ACTIVE,              /**< This taskpoint is before the active one */
    CURRENT_ACTIVE,             /**< This taskpoint is currently the active one */
    AFTER_ACTIVE                /**< This taskpoint is after the active one */
  };

protected:
  const OrderedTaskBehaviour &m_ordered_task_behaviour; /**< Reference to ordered task behaviour (for task-specific options) */

private:
  ActiveState_t m_active_state; /**< ActiveState determined from scan_active() */

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
 * @param to OrderedTask Behaviour defining options 
 * @param b_scored Whether distance within OZ is scored 
 * 
 * @return Partially initialised object 
 */
  OrderedTaskPoint(enum type _type, ObservationZonePoint* _oz,
                   const Waypoint & wp, 
                   const OrderedTaskBehaviour& to,
                   const bool b_scored=false);

  virtual ~OrderedTaskPoint() {}

  /**
   * Create a clone of the task point. 
   * Caller is responsible for destruction.
   *
   * @param task_behaviour Task behaviour of clone
   * @param ordered_task_behaviour Ordered task behaviour of clone
   * @param waypoint Waypoint to shift to (or NULL)
   */
  gcc_malloc gcc_pure
  OrderedTaskPoint* clone(const TaskBehaviour &task_behaviour,
                          const OrderedTaskBehaviour &ordered_task_behaviour,
                          const Waypoint* waypoint=NULL) const;

  /** 
   * Call this when any geometry or OZ parameters are changed
   * 
   */
  void update_oz(const TaskProjection &projection);

  /** 
   * Update observation zone geometry (or other internal data) when
   * previous/next turnpoint changes.
   */
  void update_geometry();

  /**
   * Is it possible to insert a task point before this one?
   */
  bool predecessor_allowed() const {
    return type != START;
  }

  /**
   * Is it possible to insert a task point after this one?
   */
  bool successor_allowed() const {
    return type != FINISH;
  }

/** 
 * Set previous/next task points.
 * 
 * @param prev Previous (incoming leg's origin) task point
 * @param next Next (outgoing leg's destination) task point
 */
  virtual void set_neighbours(OrderedTaskPoint* prev,
                              OrderedTaskPoint* next);

/** 
 * Accessor for previous task point
 * 
 * @return Previous task point
 */
  OrderedTaskPoint* get_previous() const {
    return tp_previous;
  }

/** 
 * Accessor for next task point
 * 
 * @return Next task point
 */
  OrderedTaskPoint* get_next() const {
    return tp_next;
  }
  
/** 
 * Accessor for activation state of this task point.
 * This is valid only after scan_active() has been called. 
 * 
 * @return Activation state of this task point
 */
  ActiveState_t getActiveState() const {
    return m_active_state;
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
  bool scan_active(OrderedTaskPoint* atp);

/** 
 * Calculate vector remaining from aircraft state.
 * If this task point is after active, uses the planned reference points
 * 
 * (This uses memento values)
 *
 * @param state Aircraft state
 * @return Vector remaining to this taskpoint (or next planned)
 */
  const GeoVector get_vector_remaining(const AIRCRAFT_STATE &state) const {
    return vector_remaining;
  }

/** 
 * Calculate vector from this task point to the aircraft.
 * If this task point is after active, returns null;
 * if this task point is before active, uses scored/best points.
 *
 * (This uses memento values)
 *
 * 
 * @return Vector from this taskpoint to aircraft (or next planned)
 */
  const GeoVector get_vector_travelled(const AIRCRAFT_STATE &) const {
    return vector_travelled;
  }

/** 
 * Calculate vector around planned task with this task point as the destination.
 * 
 * @return Vector planned to this taskpoint
 */
  const GeoVector get_vector_planned() const {
    return vector_planned;
  }

/** 
 * Test whether a taskpoint is equivalent to this one
 * 
 * For this abstract orderedtaskpoint, only compare OZ and WP
 *
 * @param other Taskpoint to compare to
 * 
 * @return True if same WP, type and OZ
 */
  virtual bool equals(const OrderedTaskPoint* other) const;

/** 
 * Update a TaskProjection to include this taskpoint and observation zone.
 * 
 * @param task_projection Projection to update
 */
  void scan_projection(TaskProjection& task_projection) const;

  /**
   * Update the bounding box in flat projected coordinates
   */
  void update_boundingbox(const TaskProjection& task_projection);

  /**
   * Test whether a boundingbox overlaps with this oz
   */
  bool boundingbox_overlaps(const FlatBoundingBox& bb) const;

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
  fixed double_leg_distance(const GeoPoint &ref) const;

private:

  bool search_nominal_if_unsampled() const;
  bool search_boundary_points() const;
};


#endif
