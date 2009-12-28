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
#ifndef SEARCH_POINT_HPP
#define SEARCH_POINT_HPP

#include "GeoPoint.hpp"
#include "Navigation/Flat/FlatGeoPoint.hpp"
#include "Navigation/ReferencePoint.hpp"

class TaskProjection;

/**
 * Class used to hold a geodetic point, its projected integer form and
 * whether or not the point is a virtual point or an actual search point.
 * The 'virtuality' of this object is currently not used. 
 */
class SearchPoint: 
  public ReferencePoint 
{
public:

/** 
 * Constructor
 * 
 * @param loc Location of search point
 * @param tp Projection used
 * @param _actual Whether search point is real or virtual
 */
  SearchPoint(const GEOPOINT &loc, const TaskProjection& tp,
    bool _actual=false);

/** 
 * Calculate projected value of geodetic coordinate
 * 
 * @param tp Projection used
 */
  void project(const TaskProjection& tp);

/** 
 * Accessor for flat projected coordinate
 * 
 * 
 * @return Flat projected coordinate
 */
  const FLAT_GEOPOINT& get_flatLocation() const {
    return flatLocation;
  };

/** 
 * Test whether two points are coincident (by their geodetic coordinates)
 * 
 * @param sp Point to compare with
 * 
 * @return True if points coincident
 */
  bool equals(const SearchPoint& sp) const;

/** 
 * Calculate flat earth distance between two points
 * 
 * @param sp Point to measure distance from
 * 
 * @return Distance in projected units
 */
  unsigned flat_distance(const SearchPoint& sp) const;

/** 
 * Rank two points according to longitude, then latitude
 * 
 * @param other Point to compare to
 * 
 * @return True if this point is further left (or if equal, lower) than the other
 */
  bool sort (const SearchPoint &other) const;

private:
  FLAT_GEOPOINT flatLocation;
  bool actual;
};


#endif
