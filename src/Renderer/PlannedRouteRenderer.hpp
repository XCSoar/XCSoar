// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Route/Route.hpp"
#include "Math/Screen.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Pen.hpp"

#include <algorithm>

/**
 * Draw the planned detour route (#DerivedInfo::planned_route) as used by the
 * main map and embedded previews (bearing line vs planned-route polyline).
 */
inline void
DrawPlannedRoutePolyline(Canvas &canvas,
                         const WindowProjection &projection,
                         const Pen &bearing_pen,
                         const StaticRoute &route) noexcept
{
  const auto r_size = route.size();
  constexpr std::size_t capacity = StaticRoute::capacity();
  BulkPixelPoint p[capacity];
  std::transform(route.begin(), route.end(), p,
                 [&projection](const auto &i) {
                   return projection.GeoToScreen(i);
                 });

  p[r_size - 1] =
    ScreenClosestPoint(p[r_size - 1], p[r_size - 2], p[r_size - 1],
                       Layout::Scale(20));

  canvas.Select(bearing_pen);
  canvas.DrawPolyline(p, r_size);
}
