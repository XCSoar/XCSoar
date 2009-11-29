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
#ifndef GEO_VECTOR_HPP
#define GEO_VECTOR_HPP

struct GEOPOINT;

bool operator != (const GEOPOINT&g1, const GEOPOINT &g2);

/**
 * A constant bearing vector in lat/lon coordinates.  
 * Should later be extended to handle
 * separately constant bearing and minimum-distance paths. 
 *
 */
struct GeoVector {
  /**
   * Constructor given supplied distance/bearing 
   */
  GeoVector(const double distance, const double bearing):
    Distance(distance),
    Bearing(bearing)
  {
  };

  /**
   * Dummy constructor given distance, 
   * used to allow GeoVector x=0 calls. 
   */
  GeoVector(const double distance):
    Distance(distance),
    Bearing(0.0)
  {
  }

  /**
   * Constructor given start and end location.  
   * Computes Distance/Bearing internally. 
   *
   * \todo
   * - handle is_average
   */
  GeoVector(const GEOPOINT &source, const GEOPOINT &target,
            const bool is_average=true);

  /**
   * Adds the distance component of a geovector 
   */
  GeoVector& operator+= (const GeoVector&g1) {
    Distance+= g1.Distance;
    return *this;
  };

  /**
   * Returns the end point of the geovector projected from the start point.  
   * Assumes constant bearing. 
   */
  GEOPOINT end_point(const GEOPOINT &source) const;

  /**
   * Returns the end point of the geovector projected from the start point.  
   * Assumes constand Bearing. 
   */
  GEOPOINT mid_point(const GEOPOINT &source) const;

  /**
   * Distance in meters 
   */
  double Distance;

  /**
   * Bearing in degrees (true north) 
   */
  double Bearing;
};

//bool operator != (const GeoVector&g1, const GeoVector &g2);

#endif
