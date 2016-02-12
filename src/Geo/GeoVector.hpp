/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef GEO_VECTOR_HPP
#define GEO_VECTOR_HPP

#include "Math/Angle.hpp"
#include "Compiler.h"

#include <type_traits>

struct GeoPoint;

/**
 * A constant bearing vector in lat/lon coordinates.  
 * Should later be extended to handle
 * separately constant bearing and minimum-distance paths. 
 *
 */
struct GeoVector {
  /** Distance in meters */
  double distance;

  /** Bearing (true north) */
  Angle bearing;

  /** Empty non-initializing constructor */
  GeoVector() = default;

  /** Constructor given supplied distance/bearing */
  constexpr
  GeoVector(double _distance, Angle _bearing)
    :distance(_distance), bearing(_bearing) {}

  /**
   * Constructor given start and end location.  
   * Computes Distance/Bearing internally. 
   */
  GeoVector(const GeoPoint &source, const GeoPoint &target);

  /**
   * Create the zero vector: zero distance, undefined bearing.
   */
  constexpr static GeoVector Zero() {
    return GeoVector(0, Angle::Zero());
  }

  /**
   * Create an invalid instance.
   */
  constexpr
  static GeoVector Invalid() {
    return GeoVector(-1, Angle::Zero());
  }

  /**
   * Returns the end point of the geovector projected from the start point.  
   * Assumes constant bearing. 
   */
  gcc_pure
  GeoPoint EndPoint(const GeoPoint &source) const;

  /**
   * Returns the end point of the geovector projected from the start point.  
   * Assumes constand Bearing. 
   *
   * @param source start of vector
   * @return location of end point
   */
  GeoPoint MidPoint(const GeoPoint &source) const;

  constexpr
  inline bool IsValid() const {
    return distance >= 0;
  }

  void SetInvalid() {
    distance = -1;
  }
};

static_assert(std::is_trivial<GeoVector>::value, "type is not trivial");

#endif
