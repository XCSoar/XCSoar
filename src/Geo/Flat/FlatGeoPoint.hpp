/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Math/fixed.hpp"
#include "Rough/RoughAltitude.hpp"
#include "Compiler.h"

/**
 * Integer projected (flat-earth) version of Geodetic coordinates
 */
struct FlatGeoPoint {
  /** Projected x (Longitude) value [undefined units] */
  int longitude;

  /** Projected y (Latitude) value [undefined units] */
  int latitude;

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
  constexpr
  FlatGeoPoint(const int x, const int y)
    :longitude(x), latitude(y) {};

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
   * Like Distance(), but shift the distance left by the specified
   * number of bits.  This method can be used to avoid integer
   * truncation errors.  Use only when you know what you're doing!
   */
  gcc_pure
  unsigned ShiftedDistance(const FlatGeoPoint &sp, unsigned bits) const;

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
  constexpr
  FlatGeoPoint operator+(const FlatGeoPoint other) const {
    return FlatGeoPoint(longitude + other.longitude,
                        latitude + other.latitude);
  }

  /**
   * Subtract one point from another
   *
   * @param p2 Point to subtract
   *
   * @return Subtracted value
   */
  constexpr
  FlatGeoPoint operator-(const FlatGeoPoint other) const {
    return FlatGeoPoint(longitude - other.longitude,
                        latitude - other.latitude);
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
    return FlatGeoPoint(iround(longitude * t),
                        iround(latitude * t));
  }

  /**
   * Calculate cross product of one point with another
   *
   * @param other That point
   *
   * @return Cross product
   */
  constexpr int CrossProduct(FlatGeoPoint other) const {
    return longitude * other.latitude - latitude * other.longitude;
  }

  /**
   * Calculate dot product of one point with another
   *
   * @param other That point
   *
   * @return Dot product
   */
  constexpr int DotProduct(FlatGeoPoint other) const {
    return longitude * other.longitude + latitude * other.latitude;
  }

  /**
   * Test whether two points are co-located
   *
   * @param other Point to compare
   *
   * @return True if coincident
   */
  constexpr
  bool operator==(const FlatGeoPoint other) const {
    return FlatGeoPoint::Equals(other);
  };

  constexpr
  bool operator!=(const FlatGeoPoint other) const {
    return !FlatGeoPoint::Equals(other);
  };

  constexpr
  bool Equals(const FlatGeoPoint sp) const {
    return longitude == sp.longitude && latitude == sp.latitude;
  }

  gcc_pure
  bool Sort(const FlatGeoPoint& sp) const {
    if (longitude < sp.longitude)
      return false;
    else if (longitude == sp.longitude)
      return latitude > sp.latitude;
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

  constexpr
  AFlatGeoPoint(const int x, const int y, const RoughAltitude alt):
    FlatGeoPoint(x,y),altitude(alt) {};

  constexpr
  AFlatGeoPoint(const FlatGeoPoint p, const RoughAltitude alt)
    :FlatGeoPoint(p), altitude(alt) {};

  constexpr
  AFlatGeoPoint():FlatGeoPoint(0,0),altitude(0) {};

  /** Rounds location to reduce state space */
  void RoundLocation() {
    // round point to correspond roughly with terrain step size
    longitude = (longitude >> 2) << 2;
    latitude = (latitude >> 2) << 2;
  }

  /**
   * Equality comparison operator
   *
   * @param other object to compare to
   *
   * @return true if location and altitude are equal
   */
  constexpr
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
  bool Sort(const AFlatGeoPoint &sp) const {
    if (!FlatGeoPoint::Sort(sp))
      return false;
    else if (FlatGeoPoint::Equals(sp))
      return altitude > sp.altitude;
    else
      return true;
  }
};

#endif
