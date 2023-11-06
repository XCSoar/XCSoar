// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ChartProjection.hpp"
#include "Geo/Flat/TaskProjection.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"

void
ChartProjection::Set(const PixelRect &rc,
                     const TaskProjection &task_projection,
                     double radius_factor) noexcept
{
  const GeoPoint center = task_projection.GetCenter();
  const auto radius = std::max(double(10000),
                               task_projection.ApproxRadius() * radius_factor);
  Set(rc, center, radius);
}

void
ChartProjection::Set(const PixelRect &rc,
                     const OrderedTaskPoint &point) noexcept
{
  GeoBounds bounds = GeoBounds::Invalid();
  point.ScanBounds(bounds);

  Set(rc, TaskProjection(bounds), 1.3);
}

void
ChartProjection::Set(const PixelRect &rc, const GeoPoint &center,
                     const double radius) noexcept
{
  SetMapRect(rc);
  SetScaleFromRadius(radius);
  SetGeoLocation(center);
  SetScreenOrigin(rc.GetCenter());
  UpdateScreenBounds();
}
