// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "FlatGeoPoint.hpp"

#include <utility>

/** Projected ray (a point and vector) in 2-d cartesian integer coordinates */
class FlatRay {
public:
  /** Origin of ray */
  FlatGeoPoint point;
  /** Vector representing ray direction and length */
  FlatGeoPoint vector;
  /** speedups for box intersection test */
  double fx;
  /** speedups for box intersection test */
  double fy;

  /**
   * Constructor given start/end locations
   *
   * @param from Origin of ray
   * @param to End point of ray
   */
  constexpr FlatRay(const FlatGeoPoint &from, const FlatGeoPoint &to) noexcept
    :point(from), vector(to - from),
     fx(vector.x != 0 ? 1.0 / vector.x : 0),
     fy(vector.y != 0 ? 1.0 / vector.y : 0) {}

  /**
   * Return the length of the ray.
   */
  [[gnu::pure]]
  int Magnitude() const noexcept;

  /**
   * Test whether two rays intersect
   *
   * @param that Other ray to test intersection with
   *
   * @return Parameter [0,1] of vector on this ray that intersection occurs (or -1 if fail)
   */
  [[gnu::pure]]
  double Intersects(const FlatRay &that) const noexcept;

  /**
   * Parametric form of ray
   *
   * @param t Parameter [0,1] of ray
   *
   * @return Location of end point
   */
  [[gnu::pure]]
  FlatGeoPoint Parametric(double t) const noexcept;

  /**
   * Determine if two rays intersect away from their nodes
   */
  [[gnu::pure]]
  bool IntersectsDistinct(const FlatRay &that) const noexcept;

  /**
   * Determine if two rays intersect away from their nodes, and return
   * the "t" parameter.  Returns a negative number if the rays to not
   * intersect.
   */
  [[gnu::pure]]
  double DistinctIntersection(const FlatRay &that) const noexcept;

private:
  /**
   * Checks whether two lines intersect or not
   *
   * @see http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
   * adapted from line_line_intersection
   */
  [[gnu::pure]]
  std::pair<int, int> IntersectsRatio(const FlatRay &that) const noexcept;
};
