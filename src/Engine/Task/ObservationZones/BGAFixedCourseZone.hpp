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

#ifndef BGAFIXEDCOURSEZONE_HPP
#define BGAFIXEDCOURSEZONE_HPP

#include "KeyholeZone.hpp"

/**
 *  A 90 degree 20km sector centered at the bisector of incoming/outgoing legs,
 *  with 500m cylinder
 */
class BGAFixedCourseZone: public KeyholeZone
{
  BGAFixedCourseZone(const BGAFixedCourseZone &other, const GeoPoint &reference)
    :KeyholeZone((const KeyholeZone &)other, reference) {}

public:  
  /** 
   * Constructor
   * 
   * @param loc Tip point of sector
   * 
   * @return Initialised object
   */
  BGAFixedCourseZone(const GeoPoint loc)
    :KeyholeZone(BGAFIXEDCOURSE, loc, fixed(20000.0))
  {
    updateSector();
  }

  ObservationZonePoint* clone(const GeoPoint *_location = NULL) const {
    if (_location)
      return new BGAFixedCourseZone(*this, *_location);

    return new BGAFixedCourseZone(*this, get_location());
  }
};

#endif
