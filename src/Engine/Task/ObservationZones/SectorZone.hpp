/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
 * Sector of finite radius, defined by segment interior to supplied start/end radials 
 */
class SectorZone: 
  public CylinderZone 
{
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
  SectorZone(const GEOPOINT &loc, 
             const fixed _radius=fixed(10000.0),
             const fixed _startRadial=fixed_zero,
             const fixed _endRadial=fixed(360.0)):
    CylinderZone(loc,_radius),
    StartRadial(_startRadial),
    EndRadial(_endRadial) 
    {
    };

  virtual ObservationZonePoint* clone(const GEOPOINT * _location=0) const {
    if (_location) {
      return new SectorZone(*_location, Radius, StartRadial, EndRadial);
    } else {
      return new SectorZone(get_location(), Radius, StartRadial, EndRadial);
    }
  }

/** 
 * Set start angle (most counter-clockwise) of sector
 * 
 * @param x Angle (deg) of radial
 */
  virtual void setStartRadial(const fixed x); 

/** 
 * Set end angle (most clockwise) of sector
 * 
 * @param x Angle (deg) of radial
 */
  virtual void setEndRadial(const fixed x); 

/** 
 * Get start radial property value
 * 
 * @return Angle (deg) of radial
 */
  fixed getStartRadial() const {
    return StartRadial;
  }

/** 
 * Get end radial property value
 * 
 * @return Angle (deg) of radial
 */
  fixed getEndRadial() const {
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
  virtual bool isInSector(const AIRCRAFT_STATE &ref) const;

/** 
 * Get point on boundary from parametric representation
 * 
 * @param t T value [0,1]
 * 
 * @return Point on boundary
 */
  GEOPOINT get_boundary_parametric(fixed t) const;  

/** 
 * Distance reduction for scoring when outside this OZ
 * 
 * @return Distance (m) to subtract from score
 */
  virtual fixed score_adjustment() const;

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
  const GEOPOINT& get_SectorStart() const {
    return SectorStart;
  }

  /** 
   * Retrieve end radial endpoint
   * 
   * @return Location of extreme point on end radial
   */
  const GEOPOINT& get_SectorEnd() const {
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
  virtual bool angleInSector(const fixed that) const;

  GEOPOINT SectorStart; /**< Location of far end point of start radial */
  GEOPOINT SectorEnd; /**< Location of far end point of end radial */

private:

  fixed StartRadial;
  fixed EndRadial;
public:
  DEFINE_VISITABLE();
};

#endif
