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

#ifndef CYLINDERZONE_HPP
#define CYLINDERZONE_HPP

#include "ObservationZonePoint.hpp"

#include <assert.h>

/**
 * Observation zone represented as a circular area with specified
 * radius from a center point
 */
class CylinderZone : public ObservationZonePoint
{
  /** radius (m) of OZ */
  fixed radius;

protected:
  CylinderZone(Shape _shape, const GeoPoint &loc,
               const fixed _radius = fixed(10000.0))
    :ObservationZonePoint(_shape, loc), radius(_radius) {
    assert(positive(radius));
  }

  CylinderZone(const CylinderZone &other, const GeoPoint &reference)
    :ObservationZonePoint((const ObservationZonePoint &)other, reference),
     radius(other.radius) {
    assert(positive(radius));
  }

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
    :ObservationZonePoint(Shape::CYLINDER, loc), radius(_radius) {
    assert(positive(radius));
  }

  static CylinderZone *CreateMatCylinderZone(const GeoPoint &loc) {
    return new CylinderZone(Shape::MAT_CYLINDER, loc, fixed(1609.344));
  }

  /**
   * Set radius property
   *
   * @param new_radius Radius (m) of cylinder
   */
  virtual void SetRadius(fixed new_radius) {
    assert(positive(new_radius));
    radius = new_radius;
  }

  /**
   * Get radius property value
   *
   * @return Radius (m) of cylinder
   */
  fixed GetRadius() const {
    return radius;
  }

  /* virtual methods from class ObservationZone */
  virtual bool IsInSector(const GeoPoint &location) const override {
    return DistanceTo(location) <= radius;
  }

  virtual bool TransitionConstraint(const GeoPoint &location,
                                    const GeoPoint &last_location) const override {
    return true;
  }

  virtual OZBoundary GetBoundary() const override;
  virtual fixed ScoreAdjustment() const override;

  /* virtual methods from class ObservationZonePoint */
  virtual bool Equals(const ObservationZonePoint &other) const override;
  virtual GeoPoint GetRandomPointInSector(const fixed mag) const override;

  virtual ObservationZonePoint *Clone(const GeoPoint &_reference) const override {
    return new CylinderZone(*this, _reference);
  }
};

#endif
