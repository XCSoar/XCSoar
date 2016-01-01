/*
  Copyright_License {

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

#ifndef TASKPOINT_HPP
#define TASKPOINT_HPP

#include "Type.hpp"
#include "Geo/GeoPoint.hpp"
#include "Compiler.h"

/**
 * Base class for all task points 
 */
class TaskPoint
{
  TaskPointType type;

  GeoPoint location;

public:
  bool IsIntermediatePoint() const {
    return type == TaskPointType::AST || type == TaskPointType::AAT;
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
  TaskPoint(TaskPointType _type, const GeoPoint &_location)
    :type(_type), location(_location) {}

  TaskPointType GetType() const {
    return type;
  }

  /**
   * Retrieve location to be used for remaining task
   * (for a pure TaskPoint, this is the reference location)
   *
   * @return Location
   */
  gcc_pure
  virtual const GeoPoint &GetLocationRemaining() const {
    return location;
  }

  /**
   * Calculate vector from aircraft to destination
   *
   * @return Vector for task leg
   */
  gcc_pure
  virtual GeoVector GetVectorRemaining(const GeoPoint &reference) const = 0;

  /**
    * Calculate vector of next leg, if there is one
    *
    * @return Vector for task leg or GeoVector::Invalid() if there is no next leg
    */
  gcc_pure
  virtual GeoVector GetNextLegVector() const;

  /**
   * Capability of this TaskPoint to have adjustable range/target
   *
   * @return True if task point has a target (can have range set)
   */
  gcc_pure
  bool HasTarget() const {
    return type == TaskPointType::AAT;
  }

  /**
   * Retrieve elevation of taskpoint, taking into account
   * rules and safety margins.
   *
   * @return Minimum allowable elevation of task point
   */
  gcc_pure
  virtual double GetElevation() const = 0;

  /**
   * distance from this to the reference
   */
  double Distance(const GeoPoint &ref) const {
    return location.Distance(ref);
  }

  /**
   * The actual location
   */
  const GeoPoint &GetLocation() const {
    return location;
  }
};

#endif
