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

#ifndef FlatGeoPoint_HPP
#define FlatGeoPoint_HPP

#include "fixed.hpp"
#include "FastMath.h"
#include "Rough/RoughAltitude.hpp"
#include "Compiler.h"

/**
 * Integer projected (flat-earth) version of Geodetic coordinates
 */
struct FlatGeoPoint {
  /** Projected x (Longitude) value [undefined units] */
  int Longitude;
  /** Projected y (Latitude) value [undefined units] */
  int Latitude;

  /**
   * Non-initialising constructor.
   */
  FlatGeoPoint() = default;

  /**
   * Constructor at specified location (x,y)
   *
   * @param x x location
   * @param y y location
   *
   * @return Initialised object at origin
   */
  gcc_constexpr_ctor
  FlatGeoPoint(const int x, const int y):
    Longitude(x),Latitude(y) {};

  /**
   * Find distance from one point to another
   *
   * @param sp That point
   *
   * @return Distance in projected units
   */
  gcc_pure
  unsigned Distance(const FlatGeoPoint &sp) const;

  /**
   * Find squared distance from one point to another
   *
   * @param sp That point
   *
   * @return Squared distance in projected units
   */
  gcc_pure
  unsigned DistanceSquared(const FlatGeoPoint &sp) const;

  /**
   * Add one point to another
   *
   * @param p2 Point to add
   *
   * @return Added value
   */
  gcc_constexpr_method
  FlatGeoPoint operator+(const FlatGeoPoint other) const {
    return FlatGeoPoint(Longitude + other.Longitude,
                        Latitude + other.Latitude);
  }

  /**
   * Subtract one point from another
   *
   * @param p2 Point to subtract
   *
   * @return Subtracted value
   */
  gcc_constexpr_method
  FlatGeoPoint operator-(const FlatGeoPoint other) const {
    return FlatGeoPoint(Longitude - other.Longitude,
                        Latitude - other.Latitude);
  }

  /**
   * Multiply point by a constant
   *
   * @param t Value to multiply
   *
   * @return Scaled value
   */
  gcc_pure
  FlatGeoPoint operator* (const fixed t) const {
    FlatGeoPoint res= *this;
    res.Longitude = iround(res.Longitude*t);
    res.Latitude = iround(res.Latitude*t);
    return res;
  }

  /**
   * Calculate cross product of one point with another
   *
   * @param other That point
   *
   * @return Cross product
   */
  gcc_pure
  int CrossProduct(const FlatGeoPoint &other) const {
    return Longitude * other.Latitude - Latitude * other.Longitude;
  }

  /**
   * Calculate dot product of one point with another
   *
   * @param other That point
   *
   * @return Dot product
   */
  gcc_pure
  int DotProduct(const FlatGeoPoint &other) const {
    return Longitude * other.Longitude + Latitude * other.Latitude;
  }

  /**
   * Test whether two points are co-located
   *
   * @param other Point to compare
   *
   * @return True if coincident
   */
  gcc_constexpr_method
  bool operator==(const FlatGeoPoint other) const {
    return FlatGeoPoint::Equals(other);
  };

  /**
   * Operator is required when SearchPoints are used in sets.
   */
  gcc_pure
  bool operator< (const FlatGeoPoint &sp) const {
    return Sort(sp);
  }

  gcc_constexpr_method
  bool Equals(const FlatGeoPoint sp) const {
    return (Longitude == sp.Longitude) && (Latitude == sp.Latitude);
  }

  gcc_pure
  bool Sort(const FlatGeoPoint& sp) const {
    if (Longitude < sp.Longitude)
      return false;
    else if (Longitude == sp.Longitude)
      return Latitude > sp.Latitude;
    else
      return true;
  }
};


/**
 * Extension of FlatGeoPoint for altitude (3d location in flat-earth space)
 */
struct AFlatGeoPoint : public FlatGeoPoint {
  /** Nav reference altitude (m) */
  RoughAltitude altitude;

  gcc_constexpr_ctor
  AFlatGeoPoint(const int x, const int y, const RoughAltitude alt):
    FlatGeoPoint(x,y),altitude(alt) {};

  gcc_constexpr_ctor
  AFlatGeoPoint(const FlatGeoPoint p, const RoughAltitude alt)
    :FlatGeoPoint(p), altitude(alt) {};

  gcc_constexpr_ctor
  AFlatGeoPoint():FlatGeoPoint(0,0),altitude(0) {};

  /** Rounds location to reduce state space */
  void RoundLocation() {
    // round point to correspond roughly with terrain step size
    Longitude = (Longitude>>2)<<2;
    Latitude = (Latitude>>2)<<2;
  }

  /**
   * Equality comparison operator
   *
   * @param other object to compare to
   *
   * @return true if location and altitude are equal
   */
  gcc_constexpr_method
  bool operator==(const AFlatGeoPoint other) const {
    return FlatGeoPoint::Equals(other) && (altitude == other.altitude);
  };

  /**
   * Ordering operator, used for set ordering.  Uses lexicographic comparison.
   *
   * @param sp object to compare to
   *
   * @return true if lexicographically smaller
   */
  gcc_pure
  bool operator< (const AFlatGeoPoint &sp) const {
    if (!Sort(sp))
      return false;
    else if (FlatGeoPoint::Equals(sp))
      return altitude > sp.altitude;
    else
      return true;
  }
};

#endif
