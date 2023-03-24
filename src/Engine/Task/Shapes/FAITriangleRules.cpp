// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FAITriangleRules.hpp"
#include "FAITriangleSettings.hpp"
#include "Geo/Math.hpp"

bool
FAITriangleRules::TestDistances(const double d1, const double d2, const double d3,
                                const FAITriangleSettings &settings) noexcept
{
  const auto d_wp = d1 + d2 + d3;

  /*
   * A triangle is a valid FAI-triangle, if no side is less than
   * 28% of the total length (total length less than 750 km), or no
   * side is less than 25% or larger than 45% of the total length
   * (totallength >= 750km).
   */

  return d_wp >= settings.GetThreshold()
    ? CheckLargeTriangle(d1, d2, d3)
    : CheckSmallTriangle(d1, d2, d3);
}

bool
FAITriangleRules::TestDistances(const GeoPoint &a, const GeoPoint &b,
                                const GeoPoint &c,
                                const FAITriangleSettings &settings) noexcept
{
  return TestDistances(Distance(a, b), Distance(b, c), Distance(c, a),
                       settings);
}
