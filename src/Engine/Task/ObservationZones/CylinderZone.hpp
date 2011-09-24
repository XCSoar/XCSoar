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

#ifndef CYLINDERZONE_HPP
#define CYLINDERZONE_HPP

#include "Task/Tasks/BaseTask/ObservationZonePoint.hpp"
#include <assert.h>

/**
 * Observation zone represented as a circular area with specified
 * radius from a center point
 */
class CylinderZone : public ObservationZonePoint
{
protected:
  /** radius (m) of OZ */
  fixed Radius;

protected:
  CylinderZone(Shape _shape, const GeoPoint &loc,
               const fixed _radius = fixed(10000.0))
    :ObservationZonePoint(_shape, loc), Radius(_radius) {}

  CylinderZone(const CylinderZone &other, const GeoPoint &reference)
    :ObservationZonePoint((const ObservationZonePoint &)other, reference),
     Radius(other.Radius) {}

public:
  /**
   * Constructor.
   *
   * @param loc Location of center
   * @param _radius Radius (m) of cylinder
   *
   * @return Initialised object
   */
  CylinderZone(const GeoPoint &loc, const fixed _radius = fixed(10000.0))
    :ObservationZonePoint(CYLINDER, loc), Radius(_radius) {}

  virtual ObservationZonePoint* clone(const GeoPoint * _location = NULL) const {
    if (_location)
      return new CylinderZone(*this, *_location);

    return new CylinderZone(*this, get_location());
  }

  /** 
   * Check whether observer is within OZ
   *
   * @return True if reference point is inside sector
   */
  virtual bool IsInSector(const AircraftState &ref) const
  {
    return distance(ref.location) <= Radius;
  }  

  /**
   * Get point on boundary from parametric representation
   *
   * @param t T value [0,1]
   *
   * @return Point on boundary
   */
  GeoPoint GetBoundaryParametric(fixed t) const;

  /**
   * Distance reduction for scoring when outside this OZ
   *
   * @return Distance (m) to subtract from score
   */
  virtual fixed ScoreAdjustment() const;

  /**
   * Check transition constraints (always true for cylinders)
   *
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if constraints are satisfied
   */
  virtual bool TransitionConstraint(const AircraftState & ref_now, 
                                    const AircraftState & ref_last) const {
    return true;
  }

  /**
   * Set radius property
   *
   * @param new_radius Radius (m) of cylinder
   */
  virtual void setRadius(fixed new_radius) {
    assert(positive(new_radius));
    Radius = new_radius;
  }

  /**
   * Get radius property value
   *
   * @return Radius (m) of cylinder
   */
  fixed getRadius() const {
    return Radius;
  }

  /**
   * Test whether an OZ is equivalent to this one
   *
   * @param other OZ to compare to
   *
   * @return True if same type and OZ parameters
   */
  virtual bool equals(const ObservationZonePoint* other) const;

  /**
   * Generate a random location inside the OZ (to be used for testing)
   *
   * @param mag proportional magnitude of error from center (0-1)
   *
   * @return Location of point
   */
  virtual GeoPoint randomPointInSector(const fixed mag) const;
};

#endif
