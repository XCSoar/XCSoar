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

#ifndef ANNULAR_SECTORZONE_HPP
#define ANNULAR_SECTORZONE_HPP

#include "SectorZone.hpp"

class AnnularSectorZone:
  public SectorZone
{
  fixed inner_radius;

protected:
  AnnularSectorZone(Shape _shape, const GeoPoint &loc,
                    const fixed _radiusOuter = fixed(10000.0),
                    const Angle _startRadial = Angle::Zero(),
                    const Angle _endRadial = Angle::FullCircle(),
                    const fixed _inner_radius = fixed(0.0))
    :SectorZone(_shape, loc, _radiusOuter, _startRadial, _endRadial),
     inner_radius(_inner_radius) {}

  AnnularSectorZone(const AnnularSectorZone &other, const GeoPoint &reference)
    :SectorZone((const SectorZone &)other, reference),
     inner_radius(other.inner_radius) {}

public:
  /**
   * Constructor
   *
   * @param loc Location of tip of sector
   * @param _radius Radius of sector (m)
   * @param _startRadial Start radial (degrees), most counter-clockwise
   * @param _endRadial End radial (degrees), most clockwise
   *
   * @return Initialised object
   */
  AnnularSectorZone(const GeoPoint &loc,
                    const fixed _radiusOuter=fixed(10000.0),
                    const Angle _startRadial = Angle::Zero(),
                    const Angle _endRadial = Angle::FullCircle(),
                    const fixed _inner_radius = fixed(0.0))
    :SectorZone(ANNULAR_SECTOR, loc, _radiusOuter, _startRadial, _endRadial),
     inner_radius(_inner_radius)
  {
    UpdateSector();
  }

  /**
   * Set inner radius of annulus
   *
   * @param x Radius (m) of inner boundary
   */
  void SetInnerRadius(const fixed new_radius) {
    inner_radius = new_radius;
    if (new_radius > GetRadius())
      CylinderZone::SetRadius(new_radius);
    UpdateSector();
  }

  /**
   * Get inner radius property value
   *
   * @return Radius (m) of inner boundary
   */
  fixed GetInnerRadius() const {
    return inner_radius;
  }

  /* virtual methods from class ObservationZone */
  virtual bool IsInSector(const GeoPoint &location) const;
  virtual GeoPoint GetBoundaryParametric(fixed t) const;
  virtual OZBoundary GetBoundary() const;

  /* virtual methods from class ObservationZonePoint */
  virtual bool Equals(const ObservationZonePoint &other) const;
  virtual ObservationZonePoint *Clone(const GeoPoint &_reference) const {
    return new AnnularSectorZone(*this, _reference);
  }

  /* virtual methods from class CylinderZone */
  virtual void SetRadius(fixed new_radius) {
    CylinderZone::SetRadius(new_radius);
    if (new_radius < inner_radius)
      inner_radius = new_radius;
    UpdateSector();
  }
};

#endif
