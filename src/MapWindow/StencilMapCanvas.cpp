// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifndef ENABLE_OPENGL

#include "StencilMapCanvas.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Projection/WindowProjection.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Geo/SearchPointVector.hpp"

StencilMapCanvas::StencilMapCanvas(Canvas &_buffer, Canvas &_stencil,
                                   const WindowProjection &_proj,
                                   const AirspaceRendererSettings &_settings)
  :clip(_proj.GetScreenBounds().Scale(1.1)),
   buffer(_buffer),
   stencil(_stencil),
   proj(_proj),
   buffer_drawn(false),
   use_stencil(false),
   settings(_settings)
{
}

StencilMapCanvas::StencilMapCanvas(const StencilMapCanvas &other)
  :clip(other.clip),
   buffer(other.buffer),
   stencil(other.stencil),
   proj(other.proj),
   buffer_drawn(other.buffer_drawn),
   use_stencil(other.use_stencil),
   settings(other.settings)
{
}

void
StencilMapCanvas::DrawSearchPointVector(const SearchPointVector &points)
{
  size_t size = points.size();
  if (size < 3)
    return;

  /* copy all SearchPointVector elements to geo_points */
  GeoPoint *geo_points = geo_points_buffer.get(size * 3);
  for (unsigned i = 0; i < size; ++i)
    geo_points[i] = points[i].GetLocation();

  /* clip them */
  size = clip.ClipPolygon(geo_points, geo_points, size);
  if (size < 3)
    /* it's completely outside the screen */
    return;

  /* draw it all */
  BulkPixelPoint *screen = pixel_points_buffer.get(size);
  for (unsigned i = 0; i < size; ++i)
    screen[i] = proj.GeoToScreen(geo_points[i]);

  buffer.DrawPolygon(&screen[0], size);
  if (use_stencil)
    stencil.DrawPolygon(&screen[0], size);
}

void
StencilMapCanvas::DrawCircle(const PixelPoint &center, unsigned radius)
{
  buffer.DrawCircle(center, radius);
  if (use_stencil)
    stencil.DrawCircle(center, radius);
}

bool
StencilMapCanvas::Commit()
{
  if (!buffer_drawn)
    return false;

  buffer_drawn = false;

  if (use_stencil) {
    buffer.CopyOr({0, 0}, proj.GetScreenSize(), stencil, {0, 0});
  }

  return true;
}

void
StencilMapCanvas::Begin()
{
  if (!buffer_drawn) {
    ClearBuffer();
    buffer_drawn = true;
  }
}

void
StencilMapCanvas::ClearBuffer()
{
  buffer.ClearWhite();

  if (use_stencil)
    stencil.ClearWhite();
}

#endif // !ENABLE_OPENGL
