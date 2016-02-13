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

#ifndef SYMMETRICSECTORZONE_HPP
#define SYMMETRICSECTORZONE_HPP

#include "SectorZone.hpp"

/** Segment centered on bisector of incoming/outgoing legs */
class SymmetricSectorZone: public SectorZone
{
  Angle sector_angle;

protected:
  /**
   * Constructor.
   *
   * @param loc Tip of sector location
   * @param radius Radius (m) of sector
   * @param angle Angle subtended by start/end radials
   *
   * @return Initialised object
   */
  SymmetricSectorZone(Shape _shape, bool _can_start_through_top,
                      bool _arc_boundary,
                      const GeoPoint &loc,
                      const double radius=10000.0,
                      const Angle angle=Angle::QuarterCircle())
    :SectorZone(_shape, _can_start_through_top, _arc_boundary,
                loc, radius),
     sector_angle(angle) {}

  SymmetricSectorZone(const SymmetricSectorZone &other,
                      const GeoPoint &reference)
    :SectorZone((const SectorZone &)other, reference),
     sector_angle(other.sector_angle) {}

public:
  SymmetricSectorZone(const GeoPoint &loc,
                      const double radius=10000.0)
    :SectorZone(Shape::SYMMETRIC_QUADRANT, true, true, loc, radius),
     sector_angle(Angle::QuarterCircle()) {
    UpdateSector();
  }

  /**
   * A 90 degree sector centered at the bisector of incoming/outgoing legs
   * \todo This really should have infinite length
   *
   * @param is_turnpoint Whether the sector is a turnpoint, or start/finish
   */
  static SymmetricSectorZone *CreateFAISectorZone(const GeoPoint loc,
                                                  const bool _is_turnpoint = true) {
    auto *oz =
      new SymmetricSectorZone(Shape::FAI_SECTOR, true, false, loc,
                              _is_turnpoint ? 10000 : 1000,
                              Angle::QuarterCircle());
      oz->UpdateSector();
    return oz;
  }

  /**
   * Create a 180 degree sector centered at the inverse of the
   * outgoing leg.
   *
   * @see http://www.gliding.co.uk/forms/competitionrules2010.pdf - page 11
   */
  static SymmetricSectorZone *CreateBGAStartSectorZone(const GeoPoint &reference) {
    auto *oz =
      new SymmetricSectorZone(Shape::BGA_START, true, true, reference,
                              5000,
                              Angle::HalfCircle());
      oz->UpdateSector();
      return oz;
  }

  /** 
   * Accessor for angle of sector (angle between start/end radials)
   * 
   * @return Angle (deg) of sector
   */
  Angle GetSectorAngle() const {
    return sector_angle;
  }

  void SetSectorAngle(Angle _angle) {
    sector_angle = _angle;
    UpdateSector();
  }

  /* virtual methods from class ObservationZonePoint */
  void SetLegs(const GeoPoint *previous, const GeoPoint *next) override;
  bool Equals(const ObservationZonePoint &other) const override;

  /* virtual methods from class ObservationZonePoint */
  ObservationZonePoint *Clone(const GeoPoint &_reference) const override {
    return new SymmetricSectorZone(*this, _reference);
  }
};

#endif
