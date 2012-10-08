/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "IntermediatePoint.hpp"

struct RangeAndRadial {
  /**
   * Thesigned range [-1,1] from near point on perimeter through
   * center to far side of the oz perimeter.
   */
  fixed range;

  /**
   * The bearing of the target [degrees].
   */
  fixed radial;

  static constexpr RangeAndRadial Zero() {
    return RangeAndRadial{fixed_zero, fixed_zero};
  }
};

/**
 * An AATPoint is an abstract IntermediatePoint,
 * can manage a target within the observation zone
 * but does not yet have an observation zone.
 *
 * \todo
 * - Elevation may vary with target shift
 */
class AATPoint: public IntermediateTaskPoint
{
  /** Location of target within OZ */
  GeoPoint target_location;
  /** Saved location of target within OZ */
  GeoPoint target_save;
  /** Whether target can float */
  bool target_locked;

public:
  /**
   * Constructor.  Initialises to unlocked target, target is
   * initially set to origin.
   *
   * @param _oz Observation zone for this task point
   * @param wp Waypoint origin of turnpoint
   * @param tb Task Behaviour defining options (esp safety heights)
   *
   * @return Partially-initialised object
   */
  AATPoint(ObservationZonePoint *_oz,
           const Waypoint &wp,
           const TaskBehaviour &tb)
    :IntermediateTaskPoint(AAT, _oz, wp, tb, true),
     target_location(wp.location),
     target_save(wp.location),
     target_locked(false)
  {
  }

  /**
   * Lock/unlock the target from automatic shifts
   *
   * @param do_lock Whether to lock the target
   */
  void LockTarget(bool do_lock) {
    target_locked = do_lock;
  }

  /**
   * Set target location explicitly
   *
   * @param loc Location of new target
   * @param override_lock If false, won't set the target if it is locked
   */
  void SetTarget(const GeoPoint &loc, const bool override_lock=false);

  /**
   * Set target location from a signed range & radial as bearing
   * referenced from the previous target
   * used by dlgTarget
   */
  void SetTarget(RangeAndRadial rar, const TaskProjection &projection);

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
  gcc_pure
  RangeAndRadial GetTargetRangeRadial(fixed old_range=fixed_zero) const;

  /**
   * Accessor to get target location
   *
   * @return Target location
   */
  const GeoPoint &GetTargetLocation() const {
    return target_location;
  }

  /**
   * Test whether aircraft has travelled close to isoline of target within threshold
   *
   * @param state Aircraft state
   * @param threshold Threshold for distance comparision (m)
   *
   * @return True if double leg distance from state is within threshold of target
   */
  gcc_pure
  bool IsCloseToTarget(const AircraftState& state,
                       const fixed threshold=fixed_zero) const;

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
  bool CheckTarget(const AircraftState& state, bool known_outside);

  /**
   * Check whether target needs to be moved and if so, to
   * perform the move, where aircraft is inside the observation zone
   * of the current active taskpoint.
   *
   * @param state Current aircraft state
   *
   * @return True if target was moved
   */
  bool CheckTargetInside(const AircraftState& state);

  /**
   * Check whether target needs to be moved and if so, to
   * perform the move, where aircraft is outside the observation zone
   * of the current active taskpoint.
   *
   * @param state Current aircraft state
   *
   * @return True if target was moved
   */
  bool CheckTargetOutside(const AircraftState& state);

public:

  /* virtual methods from class TaskPoint */
  const GeoPoint& GetLocationRemaining() const;
  virtual bool SetRange(const fixed p, const bool force_if_current);

  virtual bool IsTargetLocked() const {
    return target_locked;
  }

  virtual void SaveTarget() {
    target_save = target_location;
  }

  virtual void RestoreTarget() {
    target_location = target_save;
  }

  /* virtual methods from class SampledTaskPoint */
  virtual bool UpdateSampleNear(const AircraftState &state,
                                TaskEvents *task_events,
                                const TaskProjection &projection);
  virtual bool UpdateSampleFar(const AircraftState &state,
                               TaskEvents *task_events,
                               const TaskProjection &projection);

  /* virtual methods from class ObservationZoneClient */
  virtual fixed ScoreAdjustment() const {
    return fixed_zero;
  }

private:
  /* virtual methods from class OrderedTaskPoint */
  virtual bool Equals(const OrderedTaskPoint &other) const;
};
#endif
