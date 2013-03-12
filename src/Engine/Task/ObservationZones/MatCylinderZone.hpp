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

#ifndef XCSOAR_MAT_CYLINDER_ZONE_HPP
#define XCSOAR_MAT_CYLINDER_ZONE_HPP

#include "ObservationZonePoint.hpp"

/**
 * Observation zone represented as a circular area with specified
 * radius is fixed at 1 sm
 */
class MatCylinderZone final : public ObservationZonePoint {
protected:
  MatCylinderZone(Shape _shape, const GeoPoint &loc)
    :ObservationZonePoint(_shape, loc) {}

  MatCylinderZone(const MatCylinderZone &other, const GeoPoint &reference)
    :ObservationZonePoint((const ObservationZonePoint &)other, reference) {}

public:
  /**
   * Constructor.
   *
   * @param loc Location of center
   * @param _radius Radius (m) of cylinder
   *
   * @return Initialised object
   */
  MatCylinderZone(const GeoPoint &loc)
    :ObservationZonePoint(MAT_CYLINDER, loc) {}

  /**
   * Get radius property value
   *
   * @return Radius (m) of cylinder
   */
  constexpr static fixed GetRadius() {
    return fixed(1609.344);
  }

  /* virtual methods from class ObservationZone */
  virtual bool IsInSector(const GeoPoint &location) const override {
    return DistanceTo(location) <= GetRadius();
  }

  virtual bool TransitionConstraint(const GeoPoint &location,
                                    const GeoPoint &last_location) const override {
    return true;
  }

  virtual OZBoundary GetBoundary() const override;
  virtual fixed ScoreAdjustment() const override;

  /* virtual methods from class ObservationZonePoint */
  virtual GeoPoint GetRandomPointInSector(const fixed mag) const override;

  virtual ObservationZonePoint *Clone(const GeoPoint &_reference) const override {
    return new MatCylinderZone(*this, _reference);
  }
};

#endif
