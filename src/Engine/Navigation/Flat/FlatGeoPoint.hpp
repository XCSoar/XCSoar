/* Copyright_License {

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
#ifndef FLAT_GEOPOINT_HPP
#define FLAT_GEOPOINT_HPP

#include "Math/FastMath.h"

/**
 * Integer projected (flat-earth) version of Geodetic coordinates
 */
struct FLAT_GEOPOINT {
/** 
 * Constructor (default) at origin
 * 
 * @return Initialised object at origin
 */
  FLAT_GEOPOINT():Longitude(0),Latitude(0) {};

/** 
 * Constructor at specified location (x,y)
 * 
 * @param x x location
 * @param y y location
 *
 * @return Initialised object at origin
 */
  FLAT_GEOPOINT(const int x,
                const int y):
    Longitude(x),Latitude(y) {};

  int Longitude; /**< Projected x (Longitude) value (undefined units) */
  int Latitude; /**< Projected y (Latitude) value (undefined units) */

/** 
 * Find distance from one point to another
 * 
 * @param sp That point
 * 
 * @return Distance in projected units
 */
  unsigned distance_to(const FLAT_GEOPOINT &sp) const;

/** 
 * Find squared distance from one point to another
 * 
 * @param sp That point
 * 
 * @return Squared distance in projected units
 */
  unsigned distance_sq_to(const FLAT_GEOPOINT &sp) const;

/** 
 * Add one point to another
 * 
 * @param p2 Point to add
 * 
 * @return Added value
 */
  FLAT_GEOPOINT operator+ (const FLAT_GEOPOINT &p2) const {
    FLAT_GEOPOINT res= *this;
    res.Longitude += p2.Longitude;
    res.Latitude += p2.Latitude;
    return res;
  };

/** 
 * Subtract one point from another
 * 
 * @param p2 Point to subtract
 * 
 * @return Subtracted value
 */
  FLAT_GEOPOINT operator- (const FLAT_GEOPOINT &p2) const {
    FLAT_GEOPOINT res= *this;
    res.Longitude -= p2.Longitude;
    res.Latitude -= p2.Latitude;
    return res;
  };

/** 
 * Multiply point by a constant
 * 
 * @param t Value to multiply
 * 
 * @return Scaled value
 */
  FLAT_GEOPOINT operator* (const double t) const {
    FLAT_GEOPOINT res= *this;
    res.Longitude = (int)(res.Longitude*t);
    res.Latitude = (int)(res.Latitude*t);
    return res;
  };

/** 
 * Calculate cross product of one point with another
 * 
 * @param other That point
 * 
 * @return Cross product
 */
  int cross(const FLAT_GEOPOINT &other) const {
    return Longitude*other.Latitude-Latitude*other.Longitude;
  }

/** 
 * Calculate dot product of one point with another
 * 
 * @param other That point
 * 
 * @return Dot product
 */
  int dot(const FLAT_GEOPOINT &other) const {
    return Longitude*other.Longitude+Latitude*other.Latitude;
  }

/** 
 * Test whether two points are co-located
 * 
 * @param other Point to compare
 * 
 * @return True if coincident
 */
  bool operator== (const FLAT_GEOPOINT &other) const {
    return (Longitude == other.Longitude) && (Latitude == other.Latitude);
  };

/**
 * With this as a vector, and other as a vector, compute
 * the projected distance of this along other.
 */
  unsigned projected_distance(const FLAT_GEOPOINT &other) const;
};

#endif
