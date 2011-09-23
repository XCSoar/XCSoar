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

#ifndef KEYHOLEZONE_HPP
#define KEYHOLEZONE_HPP

#include "SymmetricSectorZone.hpp"

/**
 * A (default) 90 degree 10km sector centered at the bisector of incoming/outgoing legs,
 * with 500m cylinder
 */
class KeyholeZone: 
  public SymmetricSectorZone 
{
protected:
/**
 * Constructor.
 *
 * @param loc Tip of sector location
 * @param radius Radius (m) of sector
 *
 * @return Initialised object
 */
  KeyholeZone(enum shape _shape, const GeoPoint &loc,
              const fixed radius=fixed(10000.0),
              const Angle angle=Angle::radians(fixed_half_pi)):
    SymmetricSectorZone(_shape, loc, radius, angle) {}

  KeyholeZone(const KeyholeZone &other, const GeoPoint &reference):
    SymmetricSectorZone((const SymmetricSectorZone &)other, reference) {}

public:  
  /** 
   * Constructor
   * 
   * @param loc Tip point of sector
   * @param radius Outer radius (m)
   * 
   * @return Initialised object
   */
  KeyholeZone(const GeoPoint loc, const fixed radius=fixed(10000.0)):
    SymmetricSectorZone(KEYHOLE, loc, radius,
                        Angle::radians(fixed_half_pi))
  {
    updateSector();
  }

  ObservationZonePoint* clone(const GeoPoint * _location = 0) const {
    if (_location) {
      return new KeyholeZone(*this, *_location);
    } else {
      return new KeyholeZone(*this, get_location());
    }
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
};

#endif
