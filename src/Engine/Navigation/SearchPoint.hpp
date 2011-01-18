/* Copyright_License {

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
#ifndef SEARCH_POINT_HPP
#define SEARCH_POINT_HPP

#include "GeoPoint.hpp"
#include "Navigation/Flat/FlatGeoPoint.hpp"
#include "Navigation/ReferencePoint.hpp"

#include <assert.h>

class TaskProjection;

/**
 * Class used to hold a geodetic point, its projected integer form.
 */
class SearchPoint: 
  public ReferencePoint 
{
#ifndef NDEBUG
  bool projected;
#endif

public:
  /** 
   * Dummy constructor
   * 
   * @return Null object
   */
  SearchPoint()
#ifndef NDEBUG
    :projected(false)
#endif
  {}

  /**
   * Constructor.  The flat location is not initialized here; the
   * method project() must be called before you can use it.
   *
   * @param loc Location of search point
   * @param tp Projection used
   */
  SearchPoint(const GeoPoint &loc):ReferencePoint(loc)
#ifndef NDEBUG
                                  , projected(false)
#endif
  {}

/** 
 * Constructor
 * 
 * @param loc Location of search point
 * @param tp Projection used
 */
  SearchPoint(const GeoPoint &loc, const TaskProjection& tp);

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
  const FlatGeoPoint& get_flatLocation() const {
    assert(projected);

    return flatLocation;
  };

/** 
 * Test whether two points are coincident (by their geodetic coordinates)
 * 
 * @param sp Point to compare with
 * 
 * @return True if points coincident
 */
  gcc_pure
  bool equals(const SearchPoint& sp) const {
    return sp.get_location() == get_location();
  }

/** 
 * Calculate flat earth distance between two points
 * 
 * @param sp Point to measure distance from
 * 
 * @return Distance in projected units
 */
  gcc_pure
  unsigned flat_distance(const SearchPoint& sp) const {
    return flatLocation.distance_to(sp.flatLocation);
  }

/** 
 * Rank two points according to longitude, then latitude
 * 
 * @param other Point to compare to
 * 
 * @return True if this point is further left (or if equal, lower) than the other
 */
  gcc_pure
  bool sort (const SearchPoint &other) const {
    return get_location().sort(other.get_location());
  }

private:
  FlatGeoPoint flatLocation;
};


#endif
