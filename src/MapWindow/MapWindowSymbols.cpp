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

#include <stdlib.h>
#include <stdio.h>

void
MapWindow::DrawWind(Canvas &canvas, const RasterPoint &Start,
                    const PixelRect &rc) const
{
  if (IsPanning())
    return;

  TCHAR sTmp[12];
  static PixelSize tsize = { 0, 0 };

  if (!Calculated().wind_available)
    return;

  const SpeedVector wind = Calculated().wind;

  if (wind.norm < fixed_one)
    // JMW don't bother drawing it if not significant
    return;

  if (tsize.cx == 0) {
    canvas.select(Fonts::MapBold);
    tsize = canvas.text_size(_T("99"));
    tsize.cx = tsize.cx / 2;
  }

  canvas.select(Graphics::hpWind);
  canvas.select(Graphics::hbWind);

  int wmag = iround(4 * wind.norm);

  int kx = tsize.cx / Layout::FastScale(1) / 2;

  RasterPoint Arrow[7] = {
      { 0, -20 },
      { -6, -26 },
      { 0, -20 },
      { 6, -26 },
      { 0, -20 },
      { 8 + kx, -24 },
      { -8 - kx, -24 }
  };

  for (int i = 1; i < 4; i++)
    Arrow[i].y -= wmag;

  PolygonRotateShift(Arrow, 7, Start.x, Start.y,
                     wind.bearing - render_projection.GetScreenAngle());

  canvas.polygon(Arrow, 5);

  if (SettingsMap().WindArrowStyle == 1) {
    RasterPoint Tail[2] = {
      { 0, Layout::FastScale(-20) },
      { 0, Layout::FastScale(-26 - min(20, wmag) * 3) },
    };

    Angle angle = (wind.bearing - render_projection.GetScreenAngle()).as_bearing();
    PolygonRotateShift(Tail, 2, Start.x, Start.y, angle);

    // optionally draw dashed line
    Pen dash_pen(Pen::DASH, 1, COLOR_BLACK);
    canvas.select(dash_pen);
    canvas.line(Tail[0], Tail[1]);
  }

  _stprintf(sTmp, _T("%i"), iround(Units::ToUserWindSpeed(wind.norm)));

  canvas.set_text_color(COLOR_BLACK);

  TextInBoxMode_t style;
  style.Align = Center;
  style.Mode = Outlined;

  if (Arrow[5].y >= Arrow[6].y)
    TextInBox(canvas, sTmp, Arrow[5].x - kx, Arrow[5].y, style, rc);
  else
    TextInBox(canvas, sTmp, Arrow[6].x - kx, Arrow[6].y, style, rc);
}

void
MapWindow::DrawCompass(Canvas &canvas, const PixelRect &rc) const
{
  if (!compass_visible)
    return;

  RasterPoint Start;
  Start.y = Layout::Scale(19) + rc.top;
  Start.x = rc.right - Layout::Scale(19);

  RasterPoint Arrow[5] = { { 0, -13 }, { -6, 10 }, { 0, 4 }, { 6, 10 }, { 0, -13 } };

  canvas.select(Graphics::hpCompass);
  canvas.select(Graphics::hbCompass);

  // North arrow
  PolygonRotateShift(Arrow, 5, Start.x, Start.y,
                     Angle::zero() - render_projection.GetScreenAngle());
  canvas.polygon(Arrow, 5);
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
  if (!Basic().location_available ||
      SettingsMap().DisplayTrackBearing == dtbOff ||
      Calculated().circling)
    return;

  if (SettingsMap().DisplayTrackBearing == dtbAuto &&
      (Basic().track - Calculated().heading).as_delta().magnitude_degrees() < fixed(5))
    return;

  RasterPoint end;
  fixed x,y;
  (Basic().track - render_projection.GetScreenAngle()).sin_cos(x, y);
  end.x = aircraft_pos.x + iround(x * fixed_int_constant(400));
  end.y = aircraft_pos.y - iround(y * fixed_int_constant(400));

  canvas.select(Graphics::hpTrackBearingLine);
  canvas.line(aircraft_pos, end);
}
