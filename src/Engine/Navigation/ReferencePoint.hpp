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


#ifndef REFERENCEPOINT_HPP
#define REFERENCEPOINT_HPP

#include "GeoPoint.hpp"

/** 
 * Entity for objects that may be used as reference points for navigation 
 * \todo
 * - consider eliminating this class (may not be useful)
 */
class ReferencePoint {
public:
  ReferencePoint() {}

/**
 * Constructor
 * @param _location Location of reference point
 */
  ReferencePoint(const GeoPoint & _location) : m_location(_location) {
  };

  /** bearing from this to the reference
   */
  Angle bearing(const GeoPoint & ref) const {
    return m_location.bearing(ref);
  }

  /** distance from this to the reference
   */
  fixed distance(const GeoPoint & ref) const {
    return m_location.distance(ref);
  }

  /** The actual location
   */
  const GeoPoint & get_location() const {
    return m_location;
  };

private:
  GeoPoint m_location; /**< Local copy of reference point location */
};

#endif
