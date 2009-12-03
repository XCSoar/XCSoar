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
#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP

#include "Navigation/GeoPoint.hpp"
#include "Navigation/Flat/FlatGeoPoint.hpp"
#ifdef DO_PRINT
#include <iostream>
#endif

#include "Util/GenericVisitor.hpp"

class TaskProjection;

/**
 * Class for waypoints.  
 * This is small enough currently to be used with local copies (e.g. in a TaskPoint),
 * but this may change if we include airfield details inside.
 *
 * \todo
 * - consider having a static factory method provide the ID automatically
 *   so we know they will be unique.
 */
class Waypoint:
  public BaseVisitable<>
{
 public:

  unsigned id; /**< Unique id */
  GEOPOINT Location; /**< Geodetic location */
  FLAT_GEOPOINT FlatLocation; /**< Flat projected location */
  fixed Altitude; /**< Height AMSL (m) of waypoint terrain */

  /**
   * Function object used to provide access to coordinate values by kd-tree
   */
  struct kd_get_location {    
    typedef int result_type; /**< type of returned value */
    /**
     * Retrieve coordinate value from object given coordinate index
     * @param d Waypoint object
     * @param k index of coordinate
     *
     * @return Coordinate value
     */
    int operator() ( const Waypoint &d, const unsigned k) const {
      switch(k) {
      case 0:
        return d.FlatLocation.Longitude;
      case 1:
        return d.FlatLocation.Latitude;
      };
      return 0; 
    };
  };

  /**
   * Equality operator (by id)
   * 
   * @param wp Waypoint object to match against
   *
   * @return true if ids match
   */
  bool operator==(const Waypoint&wp) const {
    return id == wp.id;
  }

public:
  DEFINE_VISITABLE()

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const Waypoint& wp);
#endif
};


#endif
