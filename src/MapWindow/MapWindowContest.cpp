/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "MapWindow.hpp"
#include "Renderer/FAITriangleAreaRenderer.hpp"
#include "Look/MapLook.hpp"

#ifndef ENABLE_OPENGL
#include "ui/canvas/BufferCanvas.hpp"
#else
#include "ui/canvas/opengl/Scope.hpp"
#endif

static void
RenderFAISectors(Canvas &canvas, const WindowProjection &projection,
                 const GeoPoint &a, const GeoPoint &b,
                 const FAITriangleSettings &settings)
{
  RenderFAISector(canvas, projection, a, b, false, settings);
  RenderFAISector(canvas, projection, a, b, true, settings);
}

void
MapWindow::DrawContest(Canvas &canvas)
{
  const FlyingState &flying = Calculated().flight;

  if (GetMapSettings().show_fai_triangle_areas &&
      flying.release_location.IsValid() && flying.far_location.IsValid()) {
    const FAITriangleSettings &settings =
      GetMapSettings().fai_triangle_settings;

    /* draw FAI triangle areas */
    static constexpr Color fill_color = COLOR_YELLOW;
#if defined(ENABLE_OPENGL) || defined(USE_MEMORY_CANVAS)
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.Select(Brush(fill_color.WithAlpha(60)));
    canvas.Select(Pen(1, COLOR_BLACK.WithAlpha(90)));

    RenderFAISectors(canvas, render_projection,
                     flying.release_location, flying.far_location,
                     settings);
#else
    BufferCanvas buffer_canvas;
    buffer_canvas.Create(canvas);
    buffer_canvas.ClearWhite();
#ifdef HAVE_HATCHED_BRUSH
    buffer_canvas.Select(look.airspace.brushes[3]);
    buffer_canvas.SetTextColor(fill_color);
    buffer_canvas.SetBackgroundColor(COLOR_WHITE);
#else
    buffer_canvas.Select(Brush(fill_color));
#endif
    buffer_canvas.SelectBlackPen();
    RenderFAISectors(buffer_canvas, render_projection,
                     flying.release_location, flying.far_location,
                     settings);
    canvas.CopyAnd(buffer_canvas);
#endif
  }
}
