/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Renderer/BestCruiseArrowRenderer.hpp"
#include "Renderer/CompassRenderer.hpp"
#include "Renderer/TrackLineRenderer.hpp"
#include "Renderer/WindArrowRenderer.hpp"

void
MapWindow::DrawWind(Canvas &canvas, const RasterPoint &Start,
                    const PixelRect &rc) const
{
  if (!IsPanning())
    WindArrowRenderer::Draw(canvas, render_projection.GetScreenAngle(),
                            Start, rc, Calculated(), SettingsMap());
}

void
MapWindow::DrawCompass(Canvas &canvas, const PixelRect &rc) const
{
  if (compass_visible)
    CompassRenderer::Draw(canvas, render_projection.GetScreenAngle(), rc);
}

void
MapWindow::DrawBestCruiseTrack(Canvas &canvas, const RasterPoint aircraft_pos) const
{
  if (Basic().location_available)
    BestCruiseArrowRenderer::Draw(canvas, task_look,
                                  render_projection.GetScreenAngle(),
                                  aircraft_pos, Calculated());
}

void
MapWindow::DrawTrackBearing(Canvas &canvas, const RasterPoint aircraft_pos) const
{
  if (Basic().location_available)
    TrackLineRenderer::Draw(canvas, render_projection.GetScreenAngle(),
                            aircraft_pos, Basic(), Calculated(), SettingsMap());
}
