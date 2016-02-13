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

#ifndef KEYHOLEZONE_HPP
#define KEYHOLEZONE_HPP

#include "SymmetricSectorZone.hpp"

/**
 * A (default) 90 degree 10km sector centered at the bisector of
 * incoming/outgoing legs, with 500m cylinder
 */
class KeyholeZone final : public SymmetricSectorZone
{
  double inner_radius;

protected:
  /**
   * Constructor.
   *
   * @param loc Tip of sector location
   * @param radius Radius (m) of sector
   *
   * @return Initialised object
   */
  KeyholeZone(Shape _shape, bool _can_start_through_top,
              bool _arc_boundary,
              const GeoPoint &loc,
              const double radius = 10000.0,
              const Angle angle = Angle::QuarterCircle())
    :SymmetricSectorZone(_shape, _can_start_through_top, _arc_boundary,
                         loc, radius, angle),
     inner_radius(500) {}

  KeyholeZone(const KeyholeZone &other, const GeoPoint &reference)
    :SymmetricSectorZone((const SymmetricSectorZone &)other, reference),
     inner_radius(other.inner_radius) {}

public:
  static KeyholeZone *CreateCustomKeyholeZone(const GeoPoint &reference,
                                              double radius,
                                              Angle angle) {
    return new KeyholeZone(Shape::CUSTOM_KEYHOLE, true, true, reference,
                           radius, angle);
  }

  /**
   * Create a 90 degree 10km sector centered at the bisector of
   * incoming/outgoing legs, with 500m cylinder, according to DAeC
   * rules.
   */
  static KeyholeZone *CreateDAeCKeyholeZone(const GeoPoint &reference) {
    return new KeyholeZone(Shape::DAEC_KEYHOLE, true, true, reference,
                           10000,
                           Angle::QuarterCircle());
  }

  /**
   * Create a 90 degree 20km sector centered at the bisector of
   * incoming/outgoing legs, with 500m cylinder.
   */
  static KeyholeZone *CreateBGAFixedCourseZone(const GeoPoint &reference) {
    return new KeyholeZone(Shape::BGAFIXEDCOURSE, true, true, reference,
                           20000,
                           Angle::QuarterCircle());
  }

  /**
   * Create a 180 degree 10km sector centered at the bisector of
   * incoming/outgoing legs, with 500m cylinder
   */
  static KeyholeZone *CreateBGAEnhancedOptionZone(const GeoPoint &reference) {
    return new KeyholeZone(Shape::BGAENHANCEDOPTION, true, true, reference,
                           10000,
                           Angle::HalfCircle());
  }

  /**
   * Returns the radius of the small cylinder [m].
   */
  double GetInnerRadius() const {
    return inner_radius;
  }

  void SetInnerRadius(double _radius) {
    inner_radius = _radius;
  }

  /* virtual methods from class ObservationZone */
  bool IsInSector(const GeoPoint &location) const override;
  OZBoundary GetBoundary() const override;
  double ScoreAdjustment() const override;

  /* virtual methods from class ObservationZonePoint */
  ObservationZonePoint *Clone(const GeoPoint &_reference) const override {
    return new KeyholeZone(*this, _reference);
  }
};

#endif
