// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CompareProjection.hpp"
#include "WindowProjection.hpp"

CompareProjection::FourCorners::FourCorners(const WindowProjection &projection) noexcept
  :GeoQuadrilateral(projection.GetGeoQuadrilateral()) {}

[[gnu::pure]]
static double
SimpleDistance(const GeoPoint &a, const GeoPoint &b,
               const double latitude_cos) noexcept
{
  return hypot((a.longitude - b.longitude).AsDelta().Native(),
               (a.latitude - b.latitude).AsDelta().Native() * latitude_cos);
}

CompareProjection::CompareProjection(const WindowProjection &projection) noexcept
  :corners(projection),
   latitude_cos(corners.top_left.latitude.fastcosine()),
   max_delta(SimpleDistance(corners.top_left, corners.top_right,
                            latitude_cos) /
             projection.GetScreenSize().width)
{
}

bool
CompareProjection::Compare(const CompareProjection &other) const noexcept
{
  return max_delta > 0 &&
    SimpleDistance(corners.top_left, other.corners.top_left,
                   latitude_cos) <= max_delta &&
    SimpleDistance(corners.top_right, other.corners.top_right,
                   latitude_cos) <= max_delta &&
    SimpleDistance(corners.bottom_left, other.corners.bottom_left,
                   latitude_cos) <= max_delta &&
    SimpleDistance(corners.bottom_right, other.corners.bottom_right,
                   latitude_cos) <= max_delta;
}

bool
CompareProjection::CompareAndUpdate(const CompareProjection &other) noexcept
{
  if (Compare(other))
    return true;

  *this = other;
  return false;
}

