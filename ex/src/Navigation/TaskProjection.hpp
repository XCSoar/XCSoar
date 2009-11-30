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

#ifndef TASKPROJECTION_H
#define TASKPROJECTION_H

#include "GeoPoint.hpp"
#include "Flat/FlatGeoPoint.hpp"
#include "Flat/FlatPoint.hpp"

#ifdef DO_PRINT
#include <iostream>
#endif

/**
 * Class for performing Lambeth Conformal Conic projections from
 * ellipsoidal Earth points to and from projected points.  Has
 * converters for projected coordinates in integer and double types.
 */
class TaskProjection {
public:
  /** 
   * Constructor; initialises at origin (0,0)
   * 
   */
  TaskProjection();
  
/** 
 * Reset search bounds
 * 
 * @param ref Default value for initial search bounds
 */
  void reset(const GEOPOINT &ref);

/** 
 * Check a location against bounds and update them if outside.
 * This does not update the projection itself.
 * 
 * @param ref Point to check against bounds
 */
  void scan_location(const GEOPOINT &ref);

/** 
 * Update projection.
 * 
 */
  void update_fast();

/** 
 * Project a Geodetic point to an integer 2-d representation
 * 
 * @param tp Point to project
 * 
 * @return Projected point
 */
  FLAT_GEOPOINT project(const GEOPOINT& tp) const;

/** 
 * Project a Geodetic point to an floating point 2-d representation
 * 
 * @param tp Point to project
 * 
 * @return Projected point
 */
  FlatPoint fproject(const GEOPOINT& tp) const;

/** 
 * Projects an integer 2-d representation to a Geodetic point
 * 
 * @param tp Point to project
 * 
 * @return Projected point
 */
  GEOPOINT funproject(const FlatPoint& tp) const;

/** 
 * Calculates the integer flat earth distance from an actual distance
 * from a Geodetic point.  Note this is approximate.
 * 
 * @param tp Point to project
 * @param range Distance (m) from the Geodetic point
 * 
 * @return Distance in flat earth projected units
 */
  unsigned project_range(const GEOPOINT &tp, const fixed range) const;

/** 
 * Calculates the floating point flat earth distance from an actual distance
 * from a Geodetic point.  Note this is approximate.
 * 
 * @param tp Point to project
 * @param range Distance (m) from the Geodetic point
 * 
 * @return Distance in flat earth projected units
 */
  fixed fproject_range(const GEOPOINT &tp, const fixed range) const;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const TaskProjection& task_projection);
#endif

private:
  GEOPOINT location_min; /**< Lower left corner found in scan */
  GEOPOINT location_max; /**< Upper right corner found in scan */
  GEOPOINT location_mid; /**< Midpoint of boundary, used as projection center point */
  fixed cos_midloc;      /**< Cosine of the midpoint */
};

#endif //TASKPROJECTION_H
