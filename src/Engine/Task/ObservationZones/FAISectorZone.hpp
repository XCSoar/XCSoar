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

#ifndef FAISECTORZONE_HPP
#define FAISECTORZONE_HPP

#include "SymmetricSectorZone.hpp"

/**
 *  A 90 degree sector centered at the bisector of incoming/outgoing legs
 * \todo This really should have infinite length
 */
class FAISectorZone: 
  public SymmetricSectorZone 
{
public:  
  /** 
   * Constructor
   * 
   * @param loc Tip point of sector
   * @param is_turnpoint Whether the sector is a turnpoint, or start/finish
   * 
   * @return Initialised object
   */
  FAISectorZone(const GEOPOINT loc, const bool is_turnpoint=true):
    SymmetricSectorZone(FAI_SECTOR, loc,
                        fixed(1000.0 * (is_turnpoint ? 10 : 1)),
                        Angle::radians(fixed_half_pi)),
    m_is_turnpoint(is_turnpoint)
    {}

  ObservationZonePoint* clone(const GEOPOINT * _location=0) const {
    if (_location) {
      return new FAISectorZone(*_location, m_is_turnpoint);
    } else {
      return new FAISectorZone(get_location(), m_is_turnpoint);
    }
  }

private:
  const bool m_is_turnpoint;
};

#endif
