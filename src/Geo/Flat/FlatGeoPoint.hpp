// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

  constexpr bool operator==(const FlatGeoPoint &) const noexcept = default;

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

  constexpr AFlatGeoPoint() noexcept = default;

  constexpr AFlatGeoPoint(const int x, const int y, const int alt) noexcept
    :FlatGeoPoint(x,y),altitude(alt) {};

  constexpr AFlatGeoPoint(const FlatGeoPoint p, const int alt) noexcept
    :FlatGeoPoint(p), altitude(alt) {};

  /** Rounds location to reduce state space */
  void RoundLocation() noexcept {
    // round point to correspond roughly with terrain step size
    x = (x >> 2) << 2;
    y = (y >> 2) << 2;
  }

  constexpr bool operator==(const AFlatGeoPoint &) const noexcept = default;

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
