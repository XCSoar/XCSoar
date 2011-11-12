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

#ifndef TASKPOINT_HPP
#define TASKPOINT_HPP

#include "Compiler.h"
#include "Navigation/Aircraft.hpp"

struct GeoVector;
struct TaskBehaviour;

/**
 * Base class for all task points 
 */
class TaskPoint
{
public:
  friend class PrintHelper;

  enum Type {
    UNORDERED,
    START,
    AST,
    AAT,
    FINISH,
    ROUTE
  };

private:
  Type type;

  GeoPoint location;

  /** Altitude (AMSL, m) of task point terrain */
  fixed elevation;

public:
  bool IsIntermediatePoint() const {
    return type == AST || type == AAT;
  }

  /**
   * Constructor.  Location and elevation of waypoint is used
   * as the task point's reference values; a copy of the waypoint
   * is also stored to facilitate user-feedback.
   *
   * @param tb Task Behaviour defining options (esp safety heights)
   *
   * @return Initialised object
   */
  TaskPoint(Type _type, const GeoPoint &_location,
            const fixed _elevation) :
    type(_type), location(_location),
    elevation(_elevation) {}

  /**
   * Destructor.  Does nothing yet.
   */
  virtual ~TaskPoint() {};

  Type GetType() const {
    return type;
  }

  virtual void SetTaskBehaviour(const TaskBehaviour &tb) {}

protected:
  fixed GetBaseElevation() const {
    return elevation;
  }

public:
  /**
   * Retrieve location to be used for remaining task
   * (for a pure TaskPoint, this is the reference location)
   *
   * @return Location
   */
  gcc_pure
  virtual const GeoPoint& GetLocationRemaining() const {
    return location;
  }

  /**
   * Calculate vector from aircraft to destination
   *
   * @return Vector for task leg
   */
  gcc_pure
  virtual const GeoVector GetVectorRemaining(const GeoPoint &reference) const = 0;

  /**
   * Calculate vector from aircraft to destination
   *
   * @return Vector for task leg
   */
  gcc_pure
  virtual const GeoVector GetVectorPlanned() const = 0;

  /**
   * Calculate vector travelled along this leg
   *
   * @return Vector for task leg
   */
  gcc_pure
  virtual const GeoVector GetVectorTravelled() const = 0;

  /**
   * Dummy null method.
   * Set target to parametric value between min and max locations.
   * Targets are only moved for current or after taskpoints, unless
   * force_if_current is true.
   *
   * @param p Parametric range (0:1) to set target
   * @param force_if_current If current active, force range move (otherwise ignored)
   *
   * @return True if target was moved
   */
  virtual bool SetRange(const fixed p, const bool force_if_current) {
    return false;
  }

  /**
   * If this TaskPoint has the capability to adjust the
   * target/range, this indicates whether it is locked from
   * being updated by the optimizer
   * Only valid for TaskPoints where has_target() returns true
   *
   * @return True if target is locked
   *    or False if target is unlocked or tp has no target
   */
  gcc_pure
  virtual bool IsTargetLocked() const {
    return false;
  }

  /**
   * Capability of this TaskPoint to have adjustable range/target
   *
   * @return True if task point has a target (can have range set)
   */
  gcc_pure
  bool HasTarget() const {
    return type == AAT;
  }

  /**
   * Save local copy of target in case optimisation fails
   */
  virtual void SaveTarget() {}
  /**
   * Restore target from local copy
   */
  virtual void RestoreTarget() {}

  /**
   * Check whether aircraft has entered the observation zone.
   *
   * @return True if observation zone has been entered
   */
  gcc_pure
  virtual bool HasEntered() const = 0;

  /**
   * Recall aircraft state where it entered the observation zone.
   *
   * @return State at entry, or null if never entered
   */
  gcc_pure
  virtual const AircraftState& GetEnteredState() const = 0;

  /**
   * Retrieve elevation of taskpoint, taking into account
   * rules and safety margins.
   *
   * @return Minimum allowable elevation of task point
   */
  gcc_pure
  virtual fixed GetElevation() const = 0;

  /**
   * distance from this to the reference
   */
  fixed Distance(const GeoPoint &ref) const {
    return location.distance(ref);
  }

  /**
   * The actual location
   */
  const GeoPoint &GetLocation() const {
    return location;
  }
};

#endif
