// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Flat/FlatProjection.hpp"
#include "Flat/FlatEllipse.hpp"

#include <optional>
#include <utility>

/**
 * Ellipse in geodesic coordinates, defined by two foci and
 * a point on the ellipse.  Internally uses a flat-earth projection
 * to avoid complex and expensive geodesic calculations.
 */
class GeoEllipse
{
  FlatProjection projection;
  FlatEllipse ell;

public:
  /**
   * Constructor
   *
   * @param f1 Focus 1
   * @param f2 Focus 2
   * @param p Point on ellipse
   * @param _projection projection used for internal representation
   */
  GeoEllipse(const GeoPoint &f1, const GeoPoint &f2,
             const GeoPoint &p, const FlatProjection &_projection);

  /**
   * Parametric form of ellipse border
   *
   * @param t Parameter (0,1)
   *
   * @return Location of point on ellipse
   */
  [[gnu::pure]]
  GeoPoint Parametric(double t) const;

  /**
   * Calculate where a line from the first focus through a point p
   * intersects with the ellipse.
   *
   * @param p Origin of point from which to search
   * @param i1 Location of closest intersection point
   * @param i2 Location of furthest intersection point
   *
   * @return True if line intersects
   */
  [[gnu::pure]]
  std::optional<std::pair<GeoPoint, GeoPoint>> IntersectExtended(const GeoPoint &p) const noexcept;
};
