// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Angle;

/**
 * A line on a 2D plane described by two points.
 */
template<typename P>
struct Line2D {
  typedef P Point;
  typedef typename Point::scalar_type scalar_type;
  typedef typename Point::product_type product_type;

  Point a, b;

  constexpr Line2D(Point _a, Point _b):a(_a), b(_b) {}

  /**
   * Returns the vector pointing from #a to #b.
   */
  constexpr Point GetVector() const {
    return b - a;
  }

  /**
   * Calculate squared length of line
   *
   * @return Squared length
   */
  constexpr product_type GetSquaredDistance() const {
    return GetVector().MagnitudeSquared();
  }

  constexpr Point GetMiddle() const {
    return (a + b) / 2;
  }

  constexpr Point Normal() const {
    return Point(a.y - b.y, b.x - a.x);
  }

  Line2D<P> &operator+=(P delta) {
    a += delta;
    b += delta;
    return *this;
  }

  Line2D<P> &operator-=(P delta) {
    a -= delta;
    b -= delta;
    return *this;
  }

  /**
   * Return dot product of two lines (vectors)
   */
  constexpr product_type DotProduct(Line2D<P> other) const {
    return GetVector().DotProduct(other.GetVector());
  }

  constexpr product_type CrossProduct() const {
    return ::CrossProduct(a, b);
  }

  /**
   * Determine the point's location relative to the (infinite) line.
   *
   * @return positive value if #p is left of line, zero if it is on
   * the line, negative if it is right of the line
   */
  constexpr product_type LocatePoint(Point p) const {
    return ::CrossProduct(GetVector(), p - a);
  }

  /**
   * Is the given point on this (infinite) line?
   */
  constexpr bool Contains(Point p) const {
    return LocatePoint(p) == product_type(0);
  }

  /**
   * Calculate an interpolated point between #a and #b
   *
   * @param ratio the interpolation ratio where 0=#a and 1=#b
   */
  constexpr Point Interpolate(double ratio) const {
    return Point(scalar_type(a.x * (1 - ratio) + b.x * ratio),
                 scalar_type(a.y * (1 - ratio) + b.y * ratio));
  }

  /**
   * Calculate the position of the projection of #p onto this line,
   * expressed as ratio where 0=#a and 1=#b.
   */
  constexpr double ProjectedRatio(Point p) const {
    return (double)::DotProduct(GetVector(), p - a)
      / (double)GetSquaredDistance();
  }

  /**
   * Calculate the projection of #p into this line.
   */
  constexpr Point Project(Point p) const {
    return Interpolate(ProjectedRatio(p));
  }

  /**
   * Calculate the square distance from a point to the projected point
   * on an infinite line.
   */
  constexpr product_type SquareDistanceTo(Point p) const {
    return (p - Project(p)).MagnitudeSquared();
  }
};
