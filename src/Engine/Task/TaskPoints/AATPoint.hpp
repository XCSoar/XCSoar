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

#ifndef AATPOINT_HPP
#define AATPOINT_HPP

#include "Task/Tasks/BaseTask/IntermediatePoint.hpp"

/**
 * An AATPoint is an abstract IntermediatePoint,
 * can manage a target within the observation zone
 * but does not yet have an observation zone.
 *
 * \todo
 * - Elevation may vary with target shift
 */
class AATPoint : public IntermediateTaskPoint {
  friend class PrintHelper;

  GeoPoint m_target_location;      /**< Location of target within OZ */
  GeoPoint m_target_save;          /**< Saved location of target within OZ */
  bool m_target_locked;            /**< Whether target can float */
  SearchPointVector m_deadzone;

public:
/** 
 * Constructor.  Initialises to unlocked target, target is
 * initially set to origin.
 * 
 * @param _oz Observation zone for this task point
 * @param wp Waypoint origin of turnpoint
 * @param tb Task Behaviour defining options (esp safety heights)
 * @param to OrderedTask Behaviour defining options 
 * 
 * @return Partially-initialised object
 */
  AATPoint(ObservationZonePoint* _oz,
           const Waypoint & wp,
           const TaskBehaviour &tb,
           const OrderedTaskBehaviour& to) : 
    IntermediateTaskPoint(AAT, _oz, wp, tb, to, true),
    m_target_location(wp.location),
    m_target_save(wp.location),
    m_target_locked(false)
    {
    }

  virtual void reset();

/** 
 * Retrieve location to be used for remaining task
 * 
 * @return Location 
 */
  const GeoPoint& get_location_remaining() const;

/** 
 * Retrieve polygon of "scoring deadzone", the area in which
 * flight inside this area results in no additional scoring points.
 * 
 * @return Vector of deadzone points representing a closed polygon
 */
  gcc_pure
  const SearchPointVector& get_deadzone() const {
    return m_deadzone;
  }
  
/** 
 * Update sample if nearby
 *
 * @param state Aircraft state
 * @param task_events Callback class for feedback
 * 
 * @return True if internal state changed
 */
  bool update_sample_near(const AircraftState& state,
                          TaskEvents &task_events,
                          const TaskProjection &projection);

/** 
 * Update sample even if far, specialisation to move target for active
 * task point based on task behaviour rules.  Only does the target
 * move checks when this task point is currently active.
 *
 * @param state Aircraft state
 * @param task_events Callback class for feedback
 * 
 * @return True if internal state changed
 */
  bool update_sample_far(const AircraftState& state,
                         TaskEvents &task_events,
                         const TaskProjection &projection);

/** 
 * Lock/unlock the target from automatic shifts
 * 
 * @param do_lock Whether to lock the target
 */
  void target_lock(bool do_lock) {
    m_target_locked = do_lock;
  }

/** 
 * Accessor for locked state of target
 * 
 * @return True if target is locked
 */
  bool target_is_locked() const {
    return m_target_locked;
  }

/**
 * Save local copy of target in case optimisation fails
 */
    void target_save() {
        m_target_save = m_target_location;
    }
/**
 * Set target from local copy
 */
    void target_restore() {
        m_target_location = m_target_save;
    }

/** 
 * Set target location explicitly
 * 
 * @param loc Location of new target
 * @param override_lock If false, won't set the target if it is locked
 */
  void set_target(const GeoPoint &loc, const bool override_lock=false);

/**
 * Set target location from a signed range & radial as bearing
 * referenced from the previous target
 * used by dlgTarget
 *
 * @param range the signed range [-1,1] from near point on
 * perimeter through center to far side of the oz perimeter
 *
 * @param radial the bearing in degrees of the target
 */
  void set_target(const fixed range, const fixed radial,
                  const TaskProjection &projection);

/**
 * returns position of the target in signed range & radial as
 * bearing referenced from the previous target
 * used by dlgTarget.
 *
 * @param &range returns signed range [-1,1] from near point on
 * perimeter through center to far side of the oz perimeter
 *
 * @param &radial returns the bearing in degrees of
 * the target
 */
  void get_target_range_radial(fixed &range, fixed &radial) const;

/** 
 * Accessor to get target location
 * 
 * @return Target location
 */
  const GeoPoint &get_location_target() const {
    return m_target_location;
  }

/** 
 * Set target to parametric value between min and max locations.
 * Targets are only moved for current or after taskpoints, unless
 * force_if_current is true.
 * 
 * @param p Parametric range (0:1) to set target
 * @param force_if_current If current active, force range move (otherwise ignored)
 *
 * @return True if target was moved
 */
  bool set_range(const fixed p, const bool force_if_current);

/** 
 * Test whether aircraft has travelled close to isoline of target within threshold
 * 
 * @param state Aircraft state
 * @param threshold Threshold for distance comparision (m)
 * 
 * @return True if double leg distance from state is within threshold of target
 */
  bool close_to_target(const AircraftState& state,
                       const fixed threshold=fixed_zero) const;

 /** 
  * Calculate distance reduction for achieved task point,
  * to calcuate scored distance.
  * Specialisation from ObservationZoneClient to not adjust scoring distance
  * for AAT Points.
  * 
  * @return Distance reduction once achieved
  */
  virtual fixed score_adjustment() const {
    return fixed_zero;
  }

private:
/** 
 * Check whether target needs to be moved and if so, to
 * perform the move.  Makes no assumption as to whether the aircraft
 * within or outside the observation zone.
 * 
 * @param state Current aircraft state
 * @param known_outside set to true if known to be outside the sector
 * 
 * @return True if target was moved
 */
  bool check_target(const AircraftState& state, bool known_outside);

/** 
 * Check whether target needs to be moved and if so, to
 * perform the move, where aircraft is inside the observation zone
 * of the current active taskpoint.
 * 
 * @param state Current aircraft state
 * 
 * @return True if target was moved
 */
  bool check_target_inside(const AircraftState& state);

/** 
 * Check whether target needs to be moved and if so, to
 * perform the move, where aircraft is outside the observation zone
 * of the current active taskpoint.
 * 
 * @param state Current aircraft state
 * 
 * @return True if target was moved
 */
  bool check_target_outside(const AircraftState& state);

/** 
 * Test whether a taskpoint is equivalent to this one
 * 
 * @param other Taskpoint to compare to
 * 
 * @return True if same WP, type and OZ
 */
  bool equals(const OrderedTaskPoint* other) const;

  /**
   * Calculates the deadzone. Complexity is O(n+m) where n is number of
   * points in the boundary polygon, m is number of points in sample polygon
   */
  void update_deadzone(const TaskProjection &projection);
};
#endif
