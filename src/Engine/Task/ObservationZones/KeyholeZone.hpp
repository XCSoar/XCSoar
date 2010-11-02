/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
 *  A 90 degree 10km sector centered at the bisector of incoming/outgoing legs,
 *  with 500m cylinder
 */
class KeyholeZone: 
  public SymmetricSectorZone 
{
public:  
  /** 
   * Constructor
   * 
   * @param loc Tip point of sector
   * 
   * @return Initialised object
   */
  KeyholeZone(const GeoPoint loc):
    SymmetricSectorZone(KEYHOLE, loc, fixed(10000.0),
                        Angle::radians(fixed_half_pi))
    {}

  ObservationZonePoint* clone(const GeoPoint * _location=0) const {
    if (_location) {
      return new KeyholeZone(*_location);
    } else {
      return new KeyholeZone(get_location());
    }
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
  GeoPoint get_boundary_parametric(fixed t) const;  

/** 
 * Distance reduction for scoring when outside this OZ
 * 
 * @return Distance (m) to subtract from score
 */
  virtual fixed score_adjustment() const;
};

#endif
