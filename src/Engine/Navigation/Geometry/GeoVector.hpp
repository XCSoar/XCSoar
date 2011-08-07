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
#ifndef GEO_VECTOR_HPP
#define GEO_VECTOR_HPP

#include "Math/Angle.hpp"
#include "Compiler.h"

struct GeoPoint;

gcc_pure
bool operator != (const GeoPoint&g1, const GeoPoint &g2);

/**
 * A constant bearing vector in lat/lon coordinates.  
 * Should later be extended to handle
 * separately constant bearing and minimum-distance paths. 
 *
 */
struct GeoVector {
  /** Distance in meters */
  fixed Distance;

  /** Bearing (true north) */
  Angle Bearing;

  /** Empty non-initializing constructor */
  GeoVector() {}

  /** Constructor given supplied distance/bearing */
  GeoVector(const fixed distance, const Angle& bearing)
    :Distance(distance), Bearing(bearing) {}

  /**
   * Dummy constructor given distance, 
   * used to allow GeoVector x=0 calls. 
   */
  GeoVector(const fixed distance)
    :Distance(distance), Bearing(Angle::zero()) {}

  /**
   * Constructor given start and end location.  
   * Computes Distance/Bearing internally. 
   */
  GeoVector(const GeoPoint &source, const GeoPoint &target);

  /** Adds the distance component of a geovector */
  GeoVector& operator+= (const GeoVector &g1) {
    Distance += g1.Distance;
    return *this;
  }

  /**
   * Returns the end point of the geovector projected from the start point.  
   * Assumes constant bearing. 
   */
  gcc_pure
  GeoPoint end_point(const GeoPoint &source) const;

  /**
   * Returns the end point of the geovector projected from the start point.  
   * Assumes constand Bearing. 
   *
   * @param source start of vector
   * @return location of end point
   */
  GeoPoint mid_point(const GeoPoint &source) const;
  
  /**
   * Returns the location of a point from source along vector at distance
   *
   * @param source start of vector
   * @param distance Great circle distance (m) from source at which to find intermediate point
   * @return location of point
   */
  gcc_pure
  GeoPoint intermediate_point(const GeoPoint &source, const fixed distance) const;

  /**
   * Minimum distance from a point on the vector to the reference
   *
   * @param source Start of vector
   * @param ref Point to test
   *
   * @return Distance (m)
   */
  gcc_pure
  fixed minimum_distance(const GeoPoint &source, const GeoPoint &ref) const;
};

#endif
