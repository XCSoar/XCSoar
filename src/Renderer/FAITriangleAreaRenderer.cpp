// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FAITriangleAreaRenderer.hpp"
#include "Engine/Task/Shapes/FAITriangleArea.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoClip.hpp"
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

  GeoPoint clipped[FAI_TRIANGLE_SECTOR_MAX * 3],
    *clipped_end = clipped +
    GeoClip(projection.GetScreenBounds().Scale(1.1))
    .ClipPolygon(clipped, geo_points, geo_end - geo_points);

  BulkPixelPoint points[FAI_TRIANGLE_SECTOR_MAX], *p = points;
  for (GeoPoint *geo_i = clipped; geo_i != clipped_end;)
    *p++ = projection.GeoToScreen(*geo_i++);

  canvas.DrawPolygon(points, p - points);
}
