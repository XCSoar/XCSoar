// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Math/Point2D.hpp"

class Angle;

/**
 * 2-d Cartesian projected real-valued point
 */
struct FlatPoint : DoublePoint2D
{
  FlatPoint() = default;
  using DoublePoint2D::DoublePoint2D;

  /**
   * Calculate cross product of two points
   *
   * @param p2 Other point
   *
   * @return Cross product
   */
  constexpr double CrossProduct(const FlatPoint &p2) const {
    return ::CrossProduct(*this, p2);
  }

  /**
   * Multiply Y value of point
   *
   * @param a Value to multiply
   */
  void MultiplyY(const double a) {
    y *= a;
  }

  /**
   * Rotate point counter-clockwise around origin
   *
   * @param angle Angle (deg) to rotate point counter-clockwise
   */
  void Rotate(const Angle angle);

  /**
   * Calculate distance between two points
   *
   * @param p Other point
   *
   * @return Distance
   */
  [[gnu::pure]]
  double Distance(FlatPoint p) const {
    return (*this - p).Magnitude();
  }

  /**
   * Find sqrt(dx * dx + dy * dy)
   * @return Magnitude
   */
  [[gnu::pure]]
  double Magnitude() const {
    return hypot(x, y);
  }

  /**
   * Calculate dot product of one point with another
   *
   * @param other That point
   *
   * @return Dot product
   */
  constexpr double DotProduct(FlatPoint other) const {
    return ::DotProduct(*this, other);
  }

  /**
   * Scale a point
   *
   * @param p Scale
   *
   * @return Scaled point
   */
  constexpr FlatPoint operator*(double p) const {
    return { x * p, y * p };
  }

  constexpr
  FlatPoint Half() const {
    return FlatPoint(x / 2, y / 2);
  }
};
