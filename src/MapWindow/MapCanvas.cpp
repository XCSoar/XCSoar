// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapCanvas.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Layout.hpp"
#include "Math/Screen.hpp"
#include "Geo/SearchPointVector.hpp"

void
MapCanvas::DrawLine(GeoPoint a, GeoPoint b) noexcept
{
  if (!clip.ClipLine(a, b))
    return;

  canvas.DrawLine(projection.GeoToScreen(a), projection.GeoToScreen(b));
}

void
MapCanvas::DrawLineWithOffset(GeoPoint a, GeoPoint b) noexcept
{
  if (!clip.ClipLine(a, b))
    return;

  const auto p_a = projection.GeoToScreen(a);
  const auto p_b = projection.GeoToScreen(b);
  const auto p_end = ScreenClosestPoint(p_a, p_b, p_a, Layout::Scale(20));
  canvas.DrawLine(p_b, p_end);
}


void
MapCanvas::DrawCircle(const GeoPoint &center, double radius) noexcept
{
  auto screen_center = projection.GeoToScreen(center);
  unsigned screen_radius = projection.GeoToScreenDistance(radius);
  canvas.DrawCircle(screen_center, screen_radius);
}

void
MapCanvas::Project(const Projection &projection,
                   const SearchPointVector &points, BulkPixelPoint *screen) noexcept
{
  for (const auto i : points)
    *screen++ = projection.GeoToScreen(i.GetLocation());
}

bool
MapCanvas::PreparePolygon(const SearchPointVector &points) noexcept
{
  unsigned num_points = points.size();
  if (num_points < 3)
    return false;

  /* copy all SearchPointVector elements to geo_points */
  geo_points.GrowDiscard(num_points * 3);
  for (unsigned i = 0; i < num_points; ++i)
    geo_points[i] = points[i].GetLocation();

  /* clip them */
  num_raster_points = clip.ClipPolygon(geo_points.data(),
                                       geo_points.data(), num_points);
  if (num_raster_points < 3)
    /* it's completely outside the screen */
    return false;

  /* project all GeoPoints to screen coordinates */
  raster_points.GrowDiscard(num_raster_points);
  for (unsigned i = 0; i < num_raster_points; ++i)
    raster_points[i] = projection.GeoToScreen(geo_points[i]);

  return true;
}

void
MapCanvas::DrawPrepared() noexcept
{
  /* draw it all */
  canvas.DrawPolygon(raster_points.data(), num_raster_points);
}
