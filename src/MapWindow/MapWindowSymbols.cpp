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
#include "Screen/Fonts.hpp"
#include "Screen/TextInBox.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Math/Screen.hpp"
#include "Units/Units.hpp"
#include "Look/TaskLook.hpp"
#include "Util/Macros.hpp"
#include "Renderer/CompassRenderer.hpp"
#include "Renderer/TrackLineRenderer.hpp"
#include "Renderer/WindArrowRenderer.hpp"

#include <stdlib.h>
#include <stdio.h>

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
  if (!Basic().location_available ||
      !Calculated().task_stats.task_valid ||
      !Calculated().task_stats.current_leg.solution_remaining.IsOk() ||
      Calculated().task_stats.current_leg.solution_remaining.vector.Distance
      < fixed(0.010))
    return;

  if (Calculated().turn_mode == CLIMB)
    return;

  canvas.select(task_look.best_cruise_track_pen);
  canvas.select(task_look.best_cruise_track_brush);

  const Angle angle = Calculated().task_stats.current_leg.solution_remaining.cruise_track_bearing
                    - render_projection.GetScreenAngle();

  RasterPoint Arrow[] = { { -1, -40 }, { -1, -62 }, { -6, -62 }, {  0, -70 },
                    {  6, -62 }, {  1, -62 }, {  1, -40 }, { -1, -40 } };

  PolygonRotateShift(Arrow, ARRAY_SIZE(Arrow),
                     aircraft_pos.x, aircraft_pos.y, angle);

  canvas.polygon(Arrow, ARRAY_SIZE(Arrow));
}

void
MapWindow::DrawTrackBearing(Canvas &canvas, const RasterPoint aircraft_pos) const
{
  if (Basic().location_available)
    TrackLineRenderer::Draw(canvas, render_projection.GetScreenAngle(),
                            aircraft_pos, Basic(), Calculated(), SettingsMap());
}
