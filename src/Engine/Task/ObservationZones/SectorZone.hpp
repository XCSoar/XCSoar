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

#ifndef SECTORZONE_HPP
#define SECTORZONE_HPP

#include "CylinderZone.hpp"

/**
 * Sector of finite radius, defined by segment interior
 * to supplied start/end radials
 */
class SectorZone: public CylinderZone
{
  /**
   * Does the boundary include the arc, i.e. is crossing the arc
   * scored?
   */
  const bool arc_boundary;

  /** Location of far end point of start radial */
  GeoPoint sector_start;
  /** Location of far end point of end radial */
  GeoPoint sector_end;

  Angle start_radial;
  Angle end_radial;

protected:
  SectorZone(Shape _shape, bool _can_start_through_top,
             bool _arc_boundary,
             const GeoPoint &loc,
             const double _radius = 10000.0,
             const Angle _start_radial = Angle::Zero(),
             const Angle _end_radial = Angle::FullCircle())
    :CylinderZone(_shape, _can_start_through_top, loc, _radius),
     arc_boundary(_arc_boundary),
     start_radial(_start_radial), end_radial(_end_radial) {}

  SectorZone(const SectorZone &other, const GeoPoint &reference)
    :CylinderZone((const CylinderZone &)other, reference),
     arc_boundary(other.arc_boundary),
     sector_start(other.sector_start), sector_end(other.sector_end),
     start_radial(other.start_radial), end_radial(other.end_radial) {}

public:
  /**
   * Constructor
   *
   * @param loc Location of tip of sector
   * @param _radius Radius of sector (m)
   * @param _start_radial Start radial (degrees), most counter-clockwise
   * @param _end_radial End radial (degrees), most clockwise
   *
   * @return Initialised object
   */
  SectorZone(const GeoPoint &loc, const double _radius = 10000.0,
             const Angle _start_radial = Angle::Zero(),
             const Angle _end_radial = Angle::FullCircle())
    :CylinderZone(Shape::SECTOR, true, loc, _radius),
     arc_boundary(true),
     start_radial(_start_radial), end_radial(_end_radial)
  {
    UpdateSector();
  }

  /**
   * Set start angle (most counter-clockwise) of sector
   *
   * @param x Angle (deg) of radial
   */
  void SetStartRadial(const Angle x);

  /**
   * Set end angle (most clockwise) of sector
   *
   * @param x Angle (deg) of radial
   */
  void SetEndRadial(const Angle x);

  /**
   * Get start radial property value
   *
   * @return Angle (deg) of radial
   */
  Angle GetStartRadial() const {
    return start_radial;
  }

  /**
   * Get end radial property value
   *
   * @return Angle (deg) of radial
   */
  Angle GetEndRadial() const {
    return end_radial;
  }

  /** 
   * Retrieve start radial endpoint
   * 
   * @return Location of extreme point on start radial
   */
  const GeoPoint& GetSectorStart() const {
    return sector_start;
  }

  /** 
   * Retrieve end radial endpoint
   * 
   * @return Location of extreme point on end radial
   */
  const GeoPoint& GetSectorEnd() const {
    return sector_end;
  }

protected:
  /**
   * Updates sector parameters; call this if geometry changes to recalculate
   * sector_start and sector_end etc.
   *
   */
  void UpdateSector();

  /**
   * Test whether an angle is inside the sector limits
   *
   * @param that Angle to test (deg)
   *
   * @return True if that is within the start/end radials
   */
  gcc_pure
  bool IsAngleInSector(const Angle that) const;

public:
  /* virtual methods from class ObservationZone */
  bool IsInSector(const GeoPoint &location) const override;
  OZBoundary GetBoundary() const override;
  double ScoreAdjustment() const override;

  /* virtual methods from class ObservationZonePoint */
  bool Equals(const ObservationZonePoint &other) const override;
  ObservationZonePoint *Clone(const GeoPoint &_reference) const override {
    return new SectorZone(*this, _reference);
  }

  /* virtual methods from class CylinderZone */
  void SetRadius(double new_radius) override {
    CylinderZone::SetRadius(new_radius);
    UpdateSector();
  }
};

#endif
