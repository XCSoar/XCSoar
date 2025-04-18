// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FlatPoint.hpp"
#include "Math/Line2D.hpp"

#include <optional>
#include <utility>

/**
 * Defines an infinite line in real-valued cartesian coordinates,
 * with intersection methods.
 */
class FlatLine : public Line2D<FlatPoint>
{
  typedef Line2D<FlatPoint> Base;

public:
  /**
   * Constructor given known start/end points
   *
   * @param _p1 Start of line
   * @param _p2 End of line
   *
   * @return Initialised object
   */
  constexpr
  FlatLine(const FlatPoint _p1, const FlatPoint _p2):Base(_p1, _p2) {}

  /** 
   * Calculate intersections between this line
   * and a circle of specified radius centered at the origin.
   * 
   * @param r Radius of circle
   * @param i1 Returned intersection point 1
   * @param i2 Returned intersection point 2
   * 
   * @return True if more than one intersection is found
   */
  [[gnu::pure]]
  std::optional<std::pair<FlatPoint, FlatPoint>> IntersectOriginCircle(double r) const noexcept;

  /** 
   * Calculate intersections between this line
   * and a circle of specified radius centered at point c.
   * 
   * @param r Radius of circle
   * @param c Center of circle
   * @param i1 Returned intersection point 1
   * @param i2 Returned intersection point 2
   * 
   * @return True if more than one intersection is found
   */
  [[gnu::pure]]
  std::optional<std::pair<FlatPoint, FlatPoint>> IntersectCircle(double r,FlatPoint c) const noexcept;

  using Base::GetMiddle;

  /**
   * Find angle of this line starting from the x-axis counter-clockwise
   *
   * @return Angle (deg)
   */
  [[gnu::pure]]
  Angle GetAngle() const;

  using Base::GetSquaredDistance;

  /**
   * Calculate length of line
   *
   * @return Length
   */
  [[gnu::pure]]
  double GetDistance() const {
    return a.Distance(b);
  }

  constexpr FlatLine operator+(FlatPoint delta) const {
    return {a + delta, b + delta};
  }

  constexpr FlatLine operator-(FlatPoint delta) const {
    return {a - delta, b - delta};
  }

  /**
   * Rotate line clockwise around origin
   *
   * @param angle Angle (deg) to rotate line clockwise
   */
  void Rotate(const Angle angle);

  /**
   * Scale line in Y direction
   */
  void MultiplyY(const double factor) {
    a.MultiplyY(factor);
    b.MultiplyY(factor);
  }

  using Base::DotProduct;
};
