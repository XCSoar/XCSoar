/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Math/Util.hpp"
#include "Math/Point2D.hpp"

#include <type_traits>

/**
 * Integer projected (flat-earth) version of Geodetic coordinates
 */
struct FlatGeoPoint : IntPoint2D {
  FlatGeoPoint() noexcept = default;
  using IntPoint2D::IntPoint2D;

  /**
   * Find distance from one point to another
   *
   * @param sp That point
   *
   * @return Distance in projected units
   */
  [[gnu::pure]]
  unsigned Distance(const FlatGeoPoint &sp) const noexcept;

  /**
   * Find squared distance from one point to another
   *
   * @param sp That point
   *
   * @return Squared distance in projected units
   */
  [[gnu::pure]]
  unsigned DistanceSquared(const FlatGeoPoint &sp) const noexcept;

  /**
   * Multiply point by a constant
   *
   * @param t Value to multiply
   *
   * @return Scaled value
   */
  [[gnu::pure]]
  FlatGeoPoint operator*(const double t) const noexcept {
    return FlatGeoPoint(iround(x * t), iround(y * t));
  }

  /**
   * Calculate cross product of one point with another
   *
   * @param other That point
   *
   * @return Cross product
   */
  constexpr int CrossProduct(FlatGeoPoint other) const noexcept {
    return ::CrossProduct(*this, other);
  }

  /**
   * Calculate dot product of one point with another
   *
   * @param other That point
   *
   * @return Dot product
   */
  constexpr int DotProduct(FlatGeoPoint other) const noexcept {
    return ::DotProduct(*this, other);
  }

  /**
   * Test whether two points are co-located
   *
   * @param other Point to compare
   *
   * @return True if coincident
   */
  constexpr
  bool operator==(const FlatGeoPoint &other) const noexcept {
    return IntPoint2D::operator==(other);
  };

  constexpr
  bool operator!=(const FlatGeoPoint &other) const noexcept {
    return IntPoint2D::operator!=(other);
  };

  [[gnu::pure]]
  bool Sort(const FlatGeoPoint& sp) const noexcept {
    if (x < sp.x)
      return false;
    else if (x == sp.x)
      return y > sp.y;
    else
      return true;
  }
};

static_assert(std::is_trivial<FlatGeoPoint>::value, "type is not trivial");

/**
 * Extension of FlatGeoPoint for altitude (3d location in flat-earth space)
 */
struct AFlatGeoPoint : public FlatGeoPoint {
  /** Nav reference altitude (m) */
  int altitude;

  constexpr AFlatGeoPoint(const int x, const int y, const int alt) noexcept
    :FlatGeoPoint(x,y),altitude(alt) {};

  constexpr AFlatGeoPoint(const FlatGeoPoint p, const int alt) noexcept
    :FlatGeoPoint(p), altitude(alt) {};

  constexpr AFlatGeoPoint() noexcept
    :FlatGeoPoint(0,0),altitude(0) {};

  /** Rounds location to reduce state space */
  void RoundLocation() noexcept {
    // round point to correspond roughly with terrain step size
    x = (x >> 2) << 2;
    y = (y >> 2) << 2;
  }

  /**
   * Equality comparison operator
   *
   * @param other object to compare to
   *
   * @return true if location and altitude are equal
   */
  constexpr bool operator==(const AFlatGeoPoint &other) const noexcept {
    return FlatGeoPoint::operator==(other) && (altitude == other.altitude);
  };

  constexpr bool operator!=(const AFlatGeoPoint &other) const noexcept {
    return !(*this == other);
  };

  /**
   * Ordering operator, used for set ordering.  Uses lexicographic comparison.
   *
   * @param sp object to compare to
   *
   * @return true if lexicographically smaller
   */
  [[gnu::pure]]
  bool Sort(const AFlatGeoPoint &sp) const noexcept {
    if (!FlatGeoPoint::Sort(sp))
      return false;
    else if (FlatGeoPoint::operator==(sp))
      return altitude > sp.altitude;
    else
      return true;
  }
};

#endif
