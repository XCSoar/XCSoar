/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#ifndef XCSOAR_GEOPOINT_HPP
#define XCSOAR_GEOPOINT_HPP

/**
 * Geodetic coordinate expressed as Longitude and Latitude in degrees.
 * \todo - support wrap-around at 0,360 degrees longitude
 */
struct GEOPOINT {
/** 
 * Constructor (origin)
 * 
 * @return Point initialised at origin
 */
  GEOPOINT():Longitude(0.0),Latitude(0.0) {}

/** 
 * Constructor (supplied location)
 * 
 * @param _Longitude Longitude of point
 * @param _Latitude Latitude of point
 * 
 * @return Initialised object
 */
  GEOPOINT(const double _Longitude,
           const double _Latitude):Longitude(_Longitude),Latitude(_Latitude) {}

  double Longitude; /**< Longitude (deg) */
  double Latitude; /**< Latitude (deg) */

/** 
 * Find location a parametric distance along a vector from this point
 * 
 * @param delta Vector to feed in
 * @param t Parametric distance along delta to add [0,1]
 * 
 * @return Location of point
 */
  GEOPOINT parametric(const GEOPOINT &delta, const double t) const;

/** 
 * Find location interpolated from this point towards end
 * 
 * @param end Endpoint of interpolation
 * @param t Parametric distance along this to end [0,1]
 * 
 * @return Location of point
 */
  GEOPOINT interpolate(const GEOPOINT &end, const double t) const;

/** 
 * Multiply a point by a factor (used for deltas)
 * 
 * @param x Factor to magnify
 * 
 * @return Modified point
 */
  GEOPOINT operator* (const double x) const {
    GEOPOINT res = *this;
    res.Longitude *= x;
    res.Latitude *= x;
    return res;
  };

/** 
 * Add a delta to a point
 * 
 * @param delta Delta to add 
 * 
 * @return Modified point
 */
  GEOPOINT operator+ (const GEOPOINT &delta) const {
    GEOPOINT res = *this;
    res.Longitude += delta.Longitude;
    res.Latitude += delta.Latitude;
    return res;
  };

/** 
 * Subtracts a delta from a point
 * 
 * @param delta Delta to subtract 
 * 
 * @return Modified point
 */
  GEOPOINT operator- (const GEOPOINT &delta) const {
    GEOPOINT res = *this;
    res.Longitude -= delta.Longitude;
    res.Latitude -= delta.Latitude;
    return res;
  };

/** 
 * Calculate great circle distance from this to the other
 * 
 * @param other Other location
 * 
 * @return Distance (m)
 */
  double distance(const GEOPOINT &other) const;

/** 
 * Calculate great circle initial bearing from this to the other
 * 
 * @param other Other location
 * 
 * @return Bearing (deg)
 */
  double bearing(const GEOPOINT &other) const;

/** 
 * Find distance along a great-circle path that this point
 * is projected to
 * 
 * @param from Start location
 * @param to End location
 * 
 * @return Distance (m) along from-to line
 */
  double projected_distance(const GEOPOINT &from,
                            const GEOPOINT &to) const;

/** 
 * Test whether two points are co-located
 * 
 * @param other Point to compare
 * 
 * @return True if coincident
 */
  bool equals(const GEOPOINT &other) const;

/** 
 * Test whether two points are co-located
 * 
 * @param other Point to compare
 * 
 * @return True if coincident
 */
  bool operator== (const GEOPOINT &other) const {
    return equals(other);
  };

/** 
 * Rank two points according to longitude, then latitude
 * 
 * @param other Point to compare to
 * 
 * @return True if this point is further left (or if equal, lower) than the other
 */
  bool sort (const GEOPOINT &other) const;

};

#endif
