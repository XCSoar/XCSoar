// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RasterProjection.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/FAISphere.hpp"

#include <algorithm>
#include <cassert>

void
RasterProjection::Set(const GeoBounds &bounds,
                      UnsignedPoint2D size) noexcept
{
  x_scale = double(size.x) / bounds.GetWidth().Native();
  left = AngleToWidth(bounds.GetWest());

  y_scale = double(size.y) / bounds.GetHeight().Native();
  top = AngleToHeight(bounds.GetNorth());
}

double
RasterProjection::FinePixelDistance(const GeoPoint &location,
                                    unsigned pixels) const noexcept
{
  /**
   * This factor is used to reduce fixed point rounding errors.
   * x_scale and y_scale are quite large numbers, and building their
   * reciprocals may lose a lot of precision.
   */
  constexpr double FACTOR = 256;

  // must have called Set() first otherwise this is invalid
  assert(x_scale != 0);
  assert(y_scale != 0);

  Angle distance = WidthToAngle(M_SQRT2 * FACTOR * pixels);
  GeoPoint p = GeoPoint(location.longitude + distance, location.latitude);
  auto x = location.DistanceS(p);

  distance = HeightToAngle(M_SQRT2 * FACTOR * pixels);
  p = GeoPoint(location.longitude, location.latitude + distance);
  auto y = location.DistanceS(p);

  return std::max(x, y) / FACTOR;
}

unsigned
RasterProjection::DistancePixelsFine(double distance) const noexcept
{
  Angle angle = Angle::Radians(distance / FAISphere::REARTH);
  return AngleToHeight(angle);
}
