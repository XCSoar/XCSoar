// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "Renderer/FAITriangleAreaRenderer.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

static void
RenderFAISectors(Canvas &canvas, const WindowProjection &projection,
                 const GeoPoint &a, const GeoPoint &b,
                 const FAITriangleSettings &settings) noexcept
{
  RenderFAISector(canvas, projection, a, b, false, settings);
  RenderFAISector(canvas, projection, a, b, true, settings);
}

void
MapWindow::DrawContest(Canvas &canvas) noexcept
{
  const FlyingState &flying = Calculated().flight;

  if (GetMapSettings().show_fai_triangle_areas &&
      flying.release_location.IsValid() && flying.far_location.IsValid()) {
    const FAITriangleSettings &settings =
      GetMapSettings().fai_triangle_settings;

    /* draw FAI triangle areas */
    static constexpr Color fill_color = COLOR_YELLOW;
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.Select(Brush(fill_color.WithAlpha(60)));
    canvas.Select(Pen(1, COLOR_BLACK.WithAlpha(90)));

    RenderFAISectors(canvas, render_projection,
                     flying.release_location, flying.far_location,
                     settings);
  }
}
