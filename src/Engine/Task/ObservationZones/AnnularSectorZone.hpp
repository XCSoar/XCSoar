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

#ifndef ANNULAR_SECTORZONE_HPP
#define ANNULAR_SECTORZONE_HPP

#include "SectorZone.hpp"

class AnnularSectorZone:
  public SectorZone
{
  fixed InnerRadius;

protected:
  AnnularSectorZone(enum shape _shape, const GeoPoint &loc,
                    const fixed _radiusOuter = fixed(10000.0),
                    const Angle _startRadial = Angle::zero(),
                    const Angle _endRadial = Angle::radians(fixed_two_pi),
                    const fixed _InnerRadius = fixed(0.0)):
    SectorZone(_shape, loc, _radiusOuter, _startRadial, _endRadial),
    InnerRadius(_InnerRadius)
    {
    }

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
                    const Angle _startRadial = Angle::zero(),
                    const Angle _endRadial = Angle::radians(fixed_two_pi),
                    const fixed _InnerRadius = fixed(0.0)):
    SectorZone(ANNULAR_SECTOR, loc, _radiusOuter, _startRadial, _endRadial),
    InnerRadius(_InnerRadius)
  {
    updateSector();
  }

  virtual ObservationZonePoint* clone(const GeoPoint * _location=0) const {
    if (_location) {
      return new AnnularSectorZone(*_location, Radius, StartRadial, EndRadial, InnerRadius);
    } else {
      return new AnnularSectorZone(get_location(), Radius, StartRadial, EndRadial, InnerRadius);
    }
  }

  /**
   * Set inner radius of annulus
   *
   * @param x Radius (m) of inner boundary
   */
  void setInnerRadius(const fixed new_radius) {
    InnerRadius = new_radius;
    Radius = std::max(Radius, InnerRadius);
    updateSector();
  }

  /**
   * Get inner radius property value
   *
   * @return Radius (m) of inner boundary
   */
  fixed getInnerRadius() const {
    return InnerRadius;
  }

  /**
   * Set radius property
   *
   * @param new_radius Radius (m) of cylinder
   */
  virtual void setRadius(fixed new_radius) {
    CylinderZone::setRadius(new_radius);
    InnerRadius = std::min(new_radius, InnerRadius);
    updateSector();
  }

  /**
   * Check whether observer is within OZ
   *
   * @return True if reference point is inside sector
   */
  virtual bool IsInSector(const AircraftState &ref) const;

  /**
   * Get point on boundary from parametric representation
   *
   * @param t T value [0,1]
   *
   * @return Point on boundary
   */
  GeoPoint GetBoundaryParametric(fixed t) const;

  /**
   * Test whether an OZ is equivalent to this one
   *
   * @param other OZ to compare to
   *
   * @return True if same type and OZ parameters
   */
  virtual bool equals(const ObservationZonePoint* other) const;
};

#endif
