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

#ifndef SECTORZONE_HPP
#define SECTORZONE_HPP

#include "CylinderZone.hpp"

/**
 * Sector of finite radius, defined by segment interior
 * to supplied start/end radials
 */
class SectorZone: public CylinderZone
{
protected:
  /** Location of far end point of start radial */
  GeoPoint SectorStart;
  /** Location of far end point of end radial */
  GeoPoint SectorEnd;

  Angle StartRadial;
  Angle EndRadial;

protected:
  SectorZone(Shape _shape, const GeoPoint &loc,
             const fixed _radius = fixed(10000.0),
             const Angle _startRadial = Angle::zero(),
             const Angle _endRadial = Angle::radians(fixed_two_pi))
    :CylinderZone(_shape, loc, _radius),
     StartRadial(_startRadial), EndRadial(_endRadial) {}

  SectorZone(const SectorZone &other, const GeoPoint &reference)
    :CylinderZone((const CylinderZone &)other, reference),
     SectorStart(other.SectorStart), SectorEnd(other.SectorEnd),
     StartRadial(other.StartRadial), EndRadial(other.EndRadial) {}

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
  SectorZone(const GeoPoint &loc, const fixed _radius = fixed(10000.0),
             const Angle _startRadial = Angle::zero(),
             const Angle _endRadial = Angle::radians(fixed_two_pi))
    :CylinderZone(SECTOR, loc, _radius),
     StartRadial(_startRadial), EndRadial(_endRadial)
  {
    updateSector();
  }

  virtual ObservationZonePoint* clone(const GeoPoint * _location = NULL) const {
    if (_location)
      return new SectorZone(*this, *_location);

    return new SectorZone(*this, get_location());
  }

  /**
   * Set start angle (most counter-clockwise) of sector
   *
   * @param x Angle (deg) of radial
   */
  virtual void setStartRadial(const Angle x); 

  /**
   * Set end angle (most clockwise) of sector
   *
   * @param x Angle (deg) of radial
   */
  virtual void setEndRadial(const Angle x); 

  /**
   * Get start radial property value
   *
   * @return Angle (deg) of radial
   */
  Angle getStartRadial() const {
    return StartRadial;
  }

  /**
   * Get end radial property value
   *
   * @return Angle (deg) of radial
   */
  Angle getEndRadial() const {
    return EndRadial;
  }

  /**
   * Set radius property
   *
   * @param new_radius Radius (m) of cylinder
   */
  virtual void setRadius(fixed new_radius) {
    CylinderZone::setRadius(new_radius);
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
   * Distance reduction for scoring when outside this OZ
   *
   * @return Distance (m) to subtract from score
   */
  virtual fixed ScoreAdjustment() const;

  /**
   * Test whether an OZ is equivalent to this one
   *
   * @param other OZ to compare to
   *
   * @return True if same type and OZ parameters
   */
  virtual bool equals(const ObservationZonePoint* other) const;

  /** 
   * Retrieve start radial endpoint
   * 
   * @return Location of extreme point on start radial
   */
  const GeoPoint& get_SectorStart() const {
    return SectorStart;
  }

  /** 
   * Retrieve end radial endpoint
   * 
   * @return Location of extreme point on end radial
   */
  const GeoPoint& get_SectorEnd() const {
    return SectorEnd;
  }

protected:
  /**
   * Updates sector parameters; call this if geometry changes to recalculate
   * SectorStart and SectorEnd etc.
   *
   */
  virtual void updateSector();

  /**
   * Test whether an angle is inside the sector limits
   *
   * @param that Angle to test (deg)
   *
   * @return True if that is within the start/end radials
   */
  virtual bool angleInSector(const Angle that) const;
};

#endif
