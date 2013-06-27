/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "MapDrawHelper.hpp"
#include "Screen/Canvas.hpp"
#include "Projection/WindowProjection.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "MapCanvas.hpp"
#include "Geo/SearchPointVector.hpp"

#ifdef USE_GDI
#include "Screen/GDI/AlphaBlend.hpp"
#endif

MapDrawHelper::MapDrawHelper(Canvas &_canvas, Canvas &_buffer, Canvas &_stencil,
                             const WindowProjection &_proj,
                             const AirspaceRendererSettings &_settings)
  :clip(_proj.GetScreenBounds().Scale(fixed(1.1))),
   canvas(_canvas),
   buffer(_buffer),
   stencil(_stencil),
   proj(_proj),
   buffer_drawn(false),
   use_stencil(false),
   settings(_settings)
{
}

MapDrawHelper::MapDrawHelper(const MapDrawHelper &other)
  :clip(other.clip),
   canvas(other.canvas),
   buffer(other.buffer),
   stencil(other.stencil),
   proj(other.proj),
   buffer_drawn(other.buffer_drawn),
   use_stencil(other.use_stencil),
   settings(other.settings)
{
}

void 
MapDrawHelper::DrawSearchPointVector(const SearchPointVector &points)
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
  RasterPoint screen[size];
  for (unsigned i = 0; i < size; ++i)
    screen[i] = proj.GeoToScreen(geo_points[i]);

  if (!MapCanvas::IsVisible(canvas, screen, size))
    return;

  buffer.DrawPolygon(&screen[0], size);
  if (use_stencil)
    stencil.DrawPolygon(&screen[0], size);
}

void 
MapDrawHelper::DrawCircle(const RasterPoint &center, unsigned radius)
{
  buffer.DrawCircle(center.x, center.y, radius);
  if (use_stencil)
    stencil.DrawCircle(center.x, center.y, radius);
}

void 
MapDrawHelper::BufferRenderFinish() 
{
  if (buffer_drawn) {
    if (use_stencil) {
#ifdef USE_MEMORY_CANVAS
      buffer.CopyTransparentBlack(stencil);
#else
      buffer.CopyOr(stencil);
#endif
    }

#ifdef HAVE_ALPHA_BLEND
#ifdef HAVE_HATCHED_BRUSH
    if (settings.transparency && AlphaBlendAvailable())
#endif
      canvas.AlphaBlend(0, 0, canvas.GetWidth(), canvas.GetHeight(),
                           buffer,
                           0, 0, canvas.GetWidth(), canvas.GetHeight(),
                           60);
#ifdef HAVE_HATCHED_BRUSH
    else
#endif
#endif
#ifdef HAVE_HATCHED_BRUSH
      canvas.CopyAnd(buffer);
#endif

    buffer_drawn = false;
  }
}

void 
MapDrawHelper::BufferRenderStart() 
{
  if (!buffer_drawn) {
    ClearBuffer();
    buffer_drawn = true;
  }
}

void 
MapDrawHelper::ClearBuffer()
{
  buffer.ClearWhite();

  if (use_stencil)
    stencil.ClearWhite();
}

#endif // !ENABLE_OPENGL
