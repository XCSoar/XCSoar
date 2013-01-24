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

#ifndef FAISECTORZONE_HPP
#define FAISECTORZONE_HPP

#include "SymmetricSectorZone.hpp"

/**
 *  A 90 degree sector centered at the bisector of incoming/outgoing legs
 * \todo This really should have infinite length
 */
class FAISectorZone: public SymmetricSectorZone
{
  const bool is_turnpoint;

  FAISectorZone(const FAISectorZone &other, const GeoPoint &reference)
    :SymmetricSectorZone((const SymmetricSectorZone &)other, reference),
     is_turnpoint(other.is_turnpoint) {}

public:  
  /** 
   * Constructor
   * 
   * @param loc Tip point of sector
   * @param is_turnpoint Whether the sector is a turnpoint, or start/finish
   * 
   * @return Initialised object
   */
  FAISectorZone(const GeoPoint loc, const bool _is_turnpoint = true)
    :SymmetricSectorZone(FAI_SECTOR, loc,
                         fixed(1000.0 * (_is_turnpoint ? 10 : 1)),
                         Angle::QuarterCircle()),
     is_turnpoint(_is_turnpoint)
  {
    UpdateSector();
  }

  /* virtual methods from class ObservationZone */
  virtual OZBoundary GetBoundary() const override;

  /* virtual methods from class ObservationZonePoint */
  virtual ObservationZonePoint *Clone(const GeoPoint &_reference) const override {
    return new FAISectorZone(*this, _reference);
  }
};

#endif
