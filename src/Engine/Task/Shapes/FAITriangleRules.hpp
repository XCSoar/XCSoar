// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <algorithm>

struct GeoPoint;
struct FAITriangleSettings;

namespace FAITriangleRules {

/**
 * The minimum leg percentage for "small FAI triangles".
 */
static constexpr double SMALL_MIN_LEG(0.28);

/**
 * The maximum leg percentage for "small FAI triangles".  This is a
 * derived value, assuming the other two legs are as short as
 * possible.
 */
static constexpr double SMALL_MAX_LEG(1 - 2 * SMALL_MIN_LEG);

/**
 * The threshold which allows applying the "large FAI triangle"
 * rules [m].
 *
 * Don't use this variable.  Use FAITriangleSettings::GetThreshold()
 * instead.
 *
 * @see FAITriangleSettings::Threshold::FAI
 */
static constexpr double LARGE_THRESHOLD_FAI(750000);

/**
 * Relaxed threshold used by some contests such as OLC and DMSt.
 *
 * Don't use this variable.  Use FAITriangleSettings::GetThreshold()
 * instead.
 *
 * @see FAITriangleSettings::Threshold::KM500
 */
static constexpr double LARGE_THRESHOLD_500(500000);

/**
 * The minimum leg percentage for "large FAI triangles".
 */
static constexpr double LARGE_MIN_LEG(0.25);

/**
 * The maximum leg percentage for "large FAI triangles".
 */
static constexpr double LARGE_MAX_LEG(0.45);

constexpr bool CheckSmallTriangle(double a, double b, double c) noexcept
{
  const double total = a + b + c;
  const double min = total * SMALL_MIN_LEG;
  return a >= min && b >= min && c >= min;
}

constexpr bool CheckSmallTriangle(unsigned a, unsigned b, unsigned c) noexcept
{
  const auto total = a + b + c;
  const auto shortest = std::min({a, b, c});
  /* 28% */
  return shortest * 25 >= total * 7;
}

constexpr bool CheckLargeTriangle(double a, double b, double c) noexcept
{
  const double total = a + b + c;
  const double min = total * LARGE_MIN_LEG;
  if (a < min || b < min || c < min)
    return false;

  const double max = total * LARGE_MAX_LEG;
  return a <= max && b <= max && c <= max;
}

constexpr bool CheckLargeTriangle(unsigned a, unsigned b, unsigned c) noexcept
{
  const auto total = a + b + c;
  const auto [shortest, longest] = std::minmax({a, b, c});
  /* 25% / 45% */
  return shortest * 4 >= total && longest * 20 < total * 9;
}

[[gnu::pure]]
bool TestDistances(double d1, double d2, double d3,
                   const FAITriangleSettings &settings) noexcept;

[[gnu::pure]]
bool TestDistances(const GeoPoint &a, const GeoPoint &b, const GeoPoint &c,
                   const FAITriangleSettings &settings) noexcept;

} // namespace FAITriangleRules
