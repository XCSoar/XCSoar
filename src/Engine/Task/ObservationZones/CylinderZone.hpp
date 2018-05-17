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
  double radius;

public:
  static constexpr double MAT_RADIUS = 1609.344;

protected:
  CylinderZone(Shape _shape, bool _can_start_through_top,
               const GeoPoint &loc,
               const double _radius = 10000.0)
    :ObservationZonePoint(_shape, _can_start_through_top, loc),
     radius(_radius) {
    assert(radius > 0);
  }

  CylinderZone(const CylinderZone &other, const GeoPoint &reference)
    :ObservationZonePoint((const ObservationZonePoint &)other, reference),
     radius(other.radius) {
    assert(radius > 0);
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
  CylinderZone(const GeoPoint &loc, const double _radius = 10000.0)
    :ObservationZonePoint(Shape::CYLINDER, true, loc), radius(_radius) {
    assert(radius > 0);
  }

  static CylinderZone *CreateMatCylinderZone(const GeoPoint &loc) {
    return new CylinderZone(Shape::MAT_CYLINDER, true, loc, MAT_RADIUS);
  }

  /**
   * Set radius property
   *
   * @param new_radius Radius (m) of cylinder
   */
  virtual void SetRadius(double new_radius) {
    assert(new_radius > 0);
    radius = new_radius;
  }

  /**
   * Get radius property value
   *
   * @return Radius (m) of cylinder
   */
  double GetRadius() const {
    return radius;
  }

  /* virtual methods from class ObservationZone */
  bool IsInSector(const GeoPoint &location) const override {
    return DistanceTo(location) <= radius;
  }

  bool TransitionConstraint(gcc_unused const GeoPoint &location,
                            gcc_unused const GeoPoint &last_location) const override {
    return true;
  }

  OZBoundary GetBoundary() const override;
  double ScoreAdjustment() const override;

  /* virtual methods from class ObservationZonePoint */
  bool Equals(const ObservationZonePoint &other) const override;
  GeoPoint GetRandomPointInSector(const double mag) const override;

  ObservationZonePoint *Clone(const GeoPoint &_reference) const override {
    return new CylinderZone(*this, _reference);
  }
};

#endif
