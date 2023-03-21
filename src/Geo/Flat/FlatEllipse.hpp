// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FlatPoint.hpp"
#include "Math/Angle.hpp"

#include <optional>
#include <utility>

class FlatLine;

/**
 * 2-d ellipse in real-valued projected coordinates, with methods for
 * intersection tests etc.  The ellipse itself need not be axis-aligned.
 */
class FlatEllipse
{
  FlatPoint f1, f2, ap;
  FlatPoint p;
  double a;
  double b;
  Angle theta;

  Angle theta_initial;

public:
  /** 
   * Constructor.
   * 
   * @param _f1 Focus A
   * @param _f2 Focus B
   * @param _ap Any point on the ellipse
   * 
   * @return Initialised object
   */
  FlatEllipse(const FlatPoint &_f1, const FlatPoint &_f2, const FlatPoint &_ap);

  /**
   * Parametric representation of ellipse
   *
   * @param t Parameter [0,1]
   *
   * @return Location on ellipse
   */
  [[gnu::pure]]
  FlatPoint Parametric(double t) const;

  /**
   * Find intersection of line from focus 1 to p, through the ellipse
   *
   * @param p Reference point
   * @param i1 Intersection point 1 if found
   * @param i2 Intersection point 2 if found
   *
   * @return True if line intersects
   */
  [[gnu::pure]]
  std::optional<std::pair<FlatPoint, FlatPoint>> IntersectExtended(const FlatPoint &p) const noexcept;

private:
  [[gnu::pure]]
  double ab() const {
    return a / b;
  }

  [[gnu::pure]]
  double ba() const {
    return b / a;
  }

  [[gnu::pure]]
  std::optional<std::pair<FlatPoint, FlatPoint>> Intersect(const FlatLine &line) const noexcept;
};
