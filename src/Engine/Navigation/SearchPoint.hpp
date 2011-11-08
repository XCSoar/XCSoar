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
#include "Util/DebugFlag.hpp"
#include "Util/TypeTraits.hpp"

#include <assert.h>

class TaskProjection;

/**
 * Class used to hold a geodetic point, its projected integer form.
 */
class SearchPoint
{
  GeoPoint reference;
  FlatGeoPoint flatLocation;

  DebugFlag projected;

public:
  /** 
   * Dummy constructor
   * 
   * @return Null object
   */
  SearchPoint() = default;

  /**
   * Constructor.  The flat location is not initialized here; the
   * method project() must be called before you can use it.
   *
   * @param loc Location of search point
   * @param tp Projection used
   */
  SearchPoint(const GeoPoint &loc) :
    reference(loc)
  {}

  /**
   * Constructor
   *
   * @param loc Location of search point
   * @param tp Projection used
   */
  SearchPoint(const GeoPoint &loc, const TaskProjection& tp);

  /**
   * Constructor
   *
   * @param floc Location of search point
   * @param tp Projection used
   */
  SearchPoint(const FlatGeoPoint &floc, const TaskProjection& tp);

  /**
   * Calculate projected value of geodetic coordinate
   *
   * @param tp Projection used
   */
  void project(const TaskProjection& tp);

  /**
   * Accessor for flat projected coordinate
   *
   * @return Flat projected coordinate
   */
  const FlatGeoPoint& get_flatLocation() const {
    assert(projected);

    return flatLocation;
  }

  /**
   * Test whether two points are coincident (by their geodetic coordinates)
   *
   * @param sp Point to compare with
   *
   * @return True if points coincident
   */
  gcc_pure
  bool equals(const SearchPoint& sp) const {
    return sp.reference == reference;
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
    return flatLocation.Distance(sp.flatLocation);
  }

  /**
   * Calculate the "flat" square distance.  This is cheaper than
   * flat_distance(), because it does not need to calculate the square
   * root.
   */
  gcc_pure
  unsigned FlatSquareDistance(const SearchPoint& sp) const {
    return flatLocation.DistanceSquared(sp.flatLocation);
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
    return reference.Sort(other.reference);
  }

  /**
   * Operator is required when SearchPoints are used in sets.
   */
  gcc_pure
  bool operator< (const SearchPoint &other) const {
    return sort(other);
  }

  /**
   * distance from this to the reference
   */
  fixed distance(const GeoPoint & ref) const {
    return reference.Distance(ref);
  }

  /**
   * The actual location
   */
  const GeoPoint & get_location() const {
    return reference;
  }
};

static_assert(is_trivial_ndebug<SearchPoint>::value, "type is not trivial");


#endif
