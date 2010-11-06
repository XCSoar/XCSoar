/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include "Appearance.hpp"

#include <stdlib.h>
#include <stdio.h>

void
MapWindow::DrawAircraft(Canvas &canvas, const POINT aircraft_pos) const
{
  POINT Aircraft[] = {
    {1, -5},
    {1, 0},
    {14, 0},
    {14, 1},
    {1, 1},
    {1, 8},
    {4, 8},
    {4, 9},
    {-3, 9},
    {-3, 8},
    {0, 8},
    {0, 1},
    {-13, 1},
    {-13, 0},
    {0, 0},
    {0, -5},
    {1, -5},
  };

  int n = sizeof(Aircraft) / sizeof(Aircraft[0]);

  const Angle angle = Basic().Heading - render_projection.GetScreenAngle();

  PolygonRotateShift(Aircraft, n, aircraft_pos.x - 1, aircraft_pos.y, angle);

  canvas.select(Graphics::hpAircraft);
  canvas.polygon(Aircraft, n);

  canvas.black_brush();

  canvas.select(Graphics::hpAircraftBorder);
  canvas.polygon(Aircraft, n);
}

void
MapWindow::DrawWindAtAircraft2(Canvas &canvas, const POINT &Start,
                               const RECT &rc) const
{
  int i;
  TCHAR sTmp[12];
  static SIZE tsize = { 0, 0 };

  const SpeedVector wind = Basic().wind;

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

  POINT Arrow[7] = {
      { 0, -20 },
      { -6, -26 },
      { 0, -20 },
      { 6, -26 },
      { 0, -20 },
      { 8 + kx, -24 },
      { -8 - kx, -24 }
  };

  for (i = 1; i < 4; i++)
    Arrow[i].y -= wmag;

  PolygonRotateShift(Arrow, 7, Start.x, Start.y,
                     wind.bearing - render_projection.GetScreenAngle());

  canvas.polygon(Arrow, 5);

  if (SettingsMap().WindArrowStyle == 1) {
    POINT Tail[2] = {
      { 0, Layout::FastScale(-20) },
      { 0, Layout::FastScale(-26 - min(20, wmag) * 3) },
    };

    Angle angle = (wind.bearing - render_projection.GetScreenAngle()).as_bearing();
    PolygonRotateShift(Tail, 2, Start.x, Start.y, angle);

    // optionally draw dashed line
    Pen dash_pen(Pen::DASH, 1, Color::BLACK);
    canvas.select(dash_pen);
    canvas.line(Tail[0], Tail[1]);
  }

  _stprintf(sTmp, _T("%i"),
            iround(Units::ToUserUnit(wind.norm, Units::WindSpeedUnit)));

  canvas.set_text_color(Color::BLACK);

  TextInBoxMode_t TextInBoxMode;
  TextInBoxMode.Align = Center;
  TextInBoxMode.Mode = Outlined;

  if (Arrow[5].y >= Arrow[6].y)
    TextInBox(canvas, sTmp, Arrow[5].x - kx, Arrow[5].y, TextInBoxMode, rc);
  else
    TextInBox(canvas, sTmp, Arrow[6].x - kx, Arrow[6].y, TextInBoxMode, rc);
}

void
MapWindow::DrawHorizon(Canvas &canvas, const RECT &rc) const
{
  POINT Start;
  Start.y = IBLSCALE(55) + rc.top;
  Start.x = rc.right - IBLSCALE(19);

  Pen hpHorizonSky(IBLSCALE(1), Color(0x40, 0x40, 0xff));
  Brush hbHorizonSky(Color(0xA0, 0xA0, 0xff));
  Pen hpHorizonGround(IBLSCALE(1), Color(106, 55, 12));
  Brush hbHorizonGround(Color(157, 101, 60));

  static const fixed fixed_div(1.0 / 50.0);
#define fixed_89 fixed_int_constant(89)

  int radius = IBLSCALE(17);
  fixed phi = max(-fixed_89,
                  min(fixed_89, Basic().acceleration.BankAngle.value_degrees()));
  fixed alpha = fixed_rad_to_deg *
                acos(max(-fixed_one,
                     min(fixed_one,
                         Basic().acceleration.PitchAngle.value_degrees() * fixed_div)));
  fixed sphi = fixed_180 - phi;
  Angle alpha1 = Angle::degrees(sphi - alpha);
  Angle alpha2 = Angle::degrees(sphi + alpha);

  canvas.select(hpHorizonSky);
  canvas.select(hbHorizonSky);

  canvas.segment(Start.x, Start.y, radius, alpha2, alpha1, true);

  canvas.select(hpHorizonGround);
  canvas.select(hbHorizonGround);

  canvas.segment(Start.x, Start.y, radius, alpha1, alpha2, true);

  Pen dash_pen(Pen::DASH, 2, Color::BLACK);
  canvas.select(dash_pen);

  canvas.line(Start.x + radius / 2, Start.y, Start.x - radius / 2, Start.y);
  canvas.line(Start.x, Start.y - radius / 4, Start.x - radius / 2, Start.y);

  unsigned rr2p = uround(radius * fixed_sqrt_half) + IBLSCALE(1);
  unsigned rr2n = uround(radius * fixed_sqrt_half);

  canvas.black_pen();
  canvas.line(Start.x + rr2p, Start.y - rr2p, Start.x + rr2n, Start.y - rr2n);
  canvas.line(Start.x - rr2p, Start.y - rr2p, Start.x - rr2n, Start.y - rr2n);

  // JMW experimental, display stall sensor
  fixed s = max(fixed_zero, min(fixed_one, Basic().StallRatio));
  long m = (long)((rc.bottom - rc.top) * s * s);

  canvas.black_pen();
  canvas.line(rc.right - 1, rc.bottom - m, rc.right - 11, rc.bottom - m);
}

void
MapWindow::DrawCompass(Canvas &canvas, const RECT &rc) const
{
  POINT Start;

  if (Appearance.CompassAppearance == apCompassDefault) {
    Start.y = IBLSCALE(19) + rc.top;
    Start.x = rc.right - IBLSCALE(19);

    POINT Arrow[5] = { { 0, -18 }, { -6, 10 }, { 0, 0 }, { 6, 10 }, { 0, -18 } };

    canvas.select(Graphics::hpCompass);
    canvas.select(Graphics::hbCompass);

    // North arrow
    PolygonRotateShift(Arrow, 5, Start.x, Start.y,
                       Angle::native(fixed_zero) - render_projection.GetScreenAngle());
    canvas.polygon(Arrow, 5);
  } else if (Appearance.CompassAppearance == apCompassAltA) {

    static Angle lastDisplayAngle = Angle::native(fixed_zero);
    static int lastRcRight = 0;
    static POINT Arrow[5] = { { 0, -11 }, { -5, 9 }, { 0, 3 }, { 5, 9 }, { 0, -11 } };

    if (lastDisplayAngle != render_projection.GetScreenAngle() ||
        lastRcRight != rc.right) {
      Arrow[0].x = 0;
      Arrow[0].y = -11;
      Arrow[1].x = -5;
      Arrow[1].y = 9;
      Arrow[2].x = 0;
      Arrow[2].y = 3;
      Arrow[3].x = 5;
      Arrow[3].y = 9;
      Arrow[4].x = 0;
      Arrow[4].y = -11;

      Start.y = rc.top + IBLSCALE(10);
      Start.x = rc.right - IBLSCALE(11);

      // North arrow
      PolygonRotateShift(Arrow, 5, Start.x, Start.y,
                         Angle::native(fixed_zero) - render_projection.GetScreenAngle());

      lastDisplayAngle = render_projection.GetScreenAngle();
      lastRcRight = rc.right;
    }
    canvas.polygon(Arrow, 5);

    canvas.select(Graphics::hpCompass);
    canvas.polygon(Arrow, 5);
  }
}

void
MapWindow::DrawBestCruiseTrack(Canvas &canvas, const POINT aircraft_pos) const
{
  if (!Calculated().task_stats.task_valid ||
      Calculated().task_stats.current_leg.solution_remaining.Vector.Distance
      < fixed(0.010))
    return;

  canvas.select(Graphics::hpBestCruiseTrack);
  canvas.select(Graphics::hbBestCruiseTrack);

  const Angle angle = Calculated().task_stats.current_leg.solution_remaining.CruiseTrackBearing
                    - render_projection.GetScreenAngle();

  POINT Arrow[] = { { -1, -40 }, { -1, -62 }, { -6, -62 }, {  0, -70 },
                    {  6, -62 }, {  1, -62 }, {  1, -40 }, { -1, -40 } };

  PolygonRotateShift(Arrow, sizeof(Arrow) / sizeof(Arrow[0]),
                     aircraft_pos.x, aircraft_pos.y, angle);

  canvas.polygon(Arrow, sizeof(Arrow) / sizeof(Arrow[0]));
}
