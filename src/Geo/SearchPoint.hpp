/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
 
#pragma once

#include "GeoPoint.hpp"
#include "Flat/FlatGeoPoint.hpp"
#include "util/TypeTraits.hpp"

#include <cassert>

class FlatProjection;

/**
 * Class used to hold a geodetic point, its projected integer form.
 */
class SearchPoint
{
  GeoPoint location;
  FlatGeoPoint flat_location;

#ifndef NDEBUG
  bool projected = false;
#endif

public:
  /** 
   * Dummy constructor
   * 
   * @return Null object
   */
  SearchPoint() noexcept = default;

  /**
   * Constructor.  The flat location is not initialized here; the
   * method project() must be called before you can use it.
   *
   * @param loc Location of search point
   * @param tp Projection used
   */
  constexpr SearchPoint(const GeoPoint &loc) noexcept
    :location(loc)
  {}

  constexpr SearchPoint(const GeoPoint &_location,
                        const FlatGeoPoint &_flat) noexcept
    :location(_location), flat_location(_flat)
#ifndef NDEBUG
    , projected(true)
#endif
  {
  }

  /**
   * Constructor
   *
   * @param loc Location of search point
   * @param tp Projection used
   */
  SearchPoint(const GeoPoint &loc, const FlatProjection &tp) noexcept;

  /**
   * Constructor
   *
   * @param floc Location of search point
   * @param tp Projection used
   */
  SearchPoint(const FlatGeoPoint &floc, const FlatProjection &tp) noexcept;

  static constexpr SearchPoint Invalid() noexcept {
    return SearchPoint(GeoPoint::Invalid());
  }

  [[gnu::pure]]
  bool IsValid() const noexcept {
    return location.IsValid();
  }

  void SetInvalid() noexcept {
    location.SetInvalid();
#ifndef NDEBUG
    projected = false;
#endif
  }

  /**
   * Calculate projected value of geodetic coordinate
   *
   * @param tp Projection used
   */
  void Project(const FlatProjection &tp) noexcept;

  /**
   * The actual location
   */
  const GeoPoint &GetLocation() const noexcept {
    return location;
  }

  /**
   * Accessor for flat projected coordinate
   *
   * @return Flat projected coordinate
   */
  const FlatGeoPoint &GetFlatLocation() const noexcept {
    assert(projected);

    return flat_location;
  }

  /**
   * Test whether two points are coincident (by their geodetic coordinates)
   *
   * @param sp Point to compare with
   *
   * @return True if points coincident
   */
  [[gnu::pure]]
  bool Equals(const SearchPoint &sp) const noexcept {
    return sp.location == location;
  }

  /**
   * Calculate flat earth distance between two points
   *
   * @param sp Point to measure distance from
   *
   * @return Distance in projected units
   */
  [[gnu::pure]]
  unsigned FlatDistanceTo(const SearchPoint &sp) const noexcept {
    return flat_location.Distance(sp.flat_location);
  }

  /**
   * Calculate the "flat" square distance.  This is cheaper than
   * flat_distance(), because it does not need to calculate the square
   * root.
   */
  [[gnu::pure]]
  unsigned FlatSquareDistanceTo(const SearchPoint &sp) const noexcept {
    return flat_location.DistanceSquared(sp.flat_location);
  }

  /**
   * distance from this to the reference
   */
  double DistanceTo(const GeoPoint &ref) const noexcept {
    return location.Distance(ref);
  }
};

static_assert(is_trivial_ndebug<SearchPoint>::value, "type is not trivial");
