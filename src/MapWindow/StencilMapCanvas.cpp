/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef ENABLE_OPENGL

#include "StencilMapCanvas.hpp"
#include "Screen/Canvas.hpp"
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
  geo_points.GrowDiscard(size * 3);
  for (unsigned i = 0; i < size; ++i)
    geo_points[i] = points[i].GetLocation();

  /* clip them */
  size = clip.ClipPolygon(geo_points.begin(), geo_points.begin(), size);
  if (size < 3)
    /* it's completely outside the screen */
    return;

  /* draw it all */
  BulkPixelPoint screen[size];
  for (unsigned i = 0; i < size; ++i)
    screen[i] = proj.GeoToScreen(geo_points[i]);

  buffer.DrawPolygon(&screen[0], size);
  if (use_stencil)
    stencil.DrawPolygon(&screen[0], size);
}

void
StencilMapCanvas::DrawCircle(const PixelPoint &center, unsigned radius)
{
  buffer.DrawCircle(center.x, center.y, radius);
  if (use_stencil)
    stencil.DrawCircle(center.x, center.y, radius);
}

bool
StencilMapCanvas::Commit()
{
  if (!buffer_drawn)
    return false;

  buffer_drawn = false;

  if (use_stencil) {
    buffer.CopyOr(0, 0, proj.GetScreenWidth(), proj.GetScreenHeight(),
                  stencil, 0, 0);
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
