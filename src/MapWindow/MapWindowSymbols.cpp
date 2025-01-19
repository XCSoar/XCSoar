// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "Look/MapLook.hpp"
#include "Renderer/BestCruiseArrowRenderer.hpp"
#include "Renderer/CompassRenderer.hpp"
#include "Renderer/TrackLineRenderer.hpp"
#include "Renderer/WindArrowRenderer.hpp"

void
MapWindow::DrawWind(Canvas &canvas, const PixelPoint &Start,
                    const PixelRect &rc) const noexcept
{
  if (IsPanning())
    return;

  WindArrowRenderer wind_arrow_renderer(look.wind);
  wind_arrow_renderer.Draw(canvas, render_projection.GetScreenAngle(),
                           Start, rc, Calculated(), GetMapSettings());
}

void
MapWindow::DrawCompass(Canvas &canvas, const PixelRect &rc) const noexcept
{
  if (!compass_visible)
    return;

  CompassRenderer compass_renderer(look);
  compass_renderer.Draw(canvas, render_projection.GetScreenAngle(), rc);
}

void
MapWindow::DrawBestCruiseTrack(Canvas &canvas,
                               const PixelPoint aircraft_pos) const noexcept
{
  if (Basic().location_available)
    BestCruiseArrowRenderer::Draw(canvas, look.task,
                                  render_projection.GetScreenAngle(),
                                  aircraft_pos, Calculated());
}

void
MapWindow::DrawTrackBearing(Canvas &canvas, const PixelPoint aircraft_pos,
                            bool circling) const noexcept
{
  if (!Basic().location_available)
    return;

  bool wind_relative = GetMapSettings().trail.wind_drift_enabled && circling;

  TrackLineRenderer track_line_renderer(look);
  track_line_renderer.Draw(canvas, render_projection,
                           aircraft_pos, Basic(), Calculated(), GetMapSettings(),
                           wind_relative);
}
