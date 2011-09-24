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

#ifndef BGA_START_SECTOR_ZONE_HPP
#define BGA_START_SECTOR_ZONE_HPP

#include "SymmetricSectorZone.hpp"

/**
 * A 180 degree sector centered at the inverse of the outgoing leg
 * @see http://www.gliding.co.uk/forms/competitionrules2010.pdf - page 11
 */
class BGAStartSectorZone: public SymmetricSectorZone
{
  BGAStartSectorZone(const BGAStartSectorZone &other, const GeoPoint &reference)
    :SymmetricSectorZone((const SymmetricSectorZone &)other, reference) {}

public:  
  /** 
   * Constructor
   * 
   * @param loc Center point of sector
   * 
   * @return Initialised object
   */
  BGAStartSectorZone(const GeoPoint loc)
    :SymmetricSectorZone(BGA_START, loc, fixed(5000.0), Angle::radians(fixed_pi))
  {
    updateSector();
  }

  ObservationZonePoint* clone(const GeoPoint * _location = 0) const {
    if (_location)
      return new BGAStartSectorZone(*this, *_location);

    return new BGAStartSectorZone(*this, get_location());
  }
};

#endif
