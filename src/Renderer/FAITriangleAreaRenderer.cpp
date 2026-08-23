// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FAITriangleAreaRenderer.hpp"
#include "Engine/Task/Shapes/FAITriangleArea.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoClip.hpp"
#include "MapWindow/MapCanvas.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"

void
RenderFAISector(Canvas &canvas, const WindowProjection &projection,
                const GeoPoint &pt1, const GeoPoint &pt2,
                bool reverse, const FAITriangleSettings &settings) noexcept
{
  GeoPoint geo_points[FAI_TRIANGLE_SECTOR_MAX];
  GeoPoint *geo_end = GenerateFAITriangleArea(geo_points, pt1, pt2,
                                              reverse, settings);

  MapCanvas map_canvas(canvas, projection,
                       GeoClip(projection.GetScreenBounds().Scale(1.1)));
  map_canvas.DrawPolygon(geo_points, geo_end - geo_points);
}
