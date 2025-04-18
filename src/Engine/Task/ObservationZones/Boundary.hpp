// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <forward_list>

class OZBoundary : public std::forward_list<GeoPoint> {
public:
  /**
   * Generate boundary points for the arc described by the parameters.
   * This excludes the points at the start/end angle.
   *
   * The arc is generated in an increasing angle order - i.e. CW.
   *
   * @param center The origin of the radials - phi, lambda
   * @param radius The radius of the arc - meters.
   * @param start_radial The most CCW portion of the arc - radians.
   * @param end_radial The most CW portion of the arc - radians.
   */
  void GenerateArcExcluding(const GeoPoint &center, double radius,
                            Angle start_radial, Angle end_radial) noexcept;
};
