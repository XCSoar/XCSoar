/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Screen/UnitSymbol.hpp"
#include "Math/Screen.hpp"
#include "Math/Constants.h"
#include "Logger/Logger.hpp"
#include "Language.hpp"
#include "Appearance.hpp"
#include "Task/ProtectedTaskManager.hpp"

#include <stdlib.h>
#include <stdio.h>

void
MapWindow::DrawCrossHairs(Canvas &canvas) const
{
  Pen dash_pen(Pen::DASH, 1, Color(50, 50, 50));
  canvas.select(dash_pen);

  const POINT Orig_Screen = render_projection.GetOrigScreen();

  canvas.line(Orig_Screen.x + 20, Orig_Screen.y,
              Orig_Screen.x - 20, Orig_Screen.y);
  canvas.line(Orig_Screen.x, Orig_Screen.y + 20,
              Orig_Screen.x, Orig_Screen.y - 20);
}

void
MapWindow::DrawAircraft(Canvas &canvas) const
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

  const Angle angle = render_projection.GetDisplayAircraftAngle() +
                      (Basic().Heading - Basic().TrackBearing);

  PolygonRotateShift(Aircraft, n, render_projection.GetOrigAircraft().x - 1,
                     render_projection.GetOrigAircraft().y, angle);

  canvas.select(Graphics::hpAircraft);
  canvas.polygon(Aircraft, n);

  canvas.black_brush();

  canvas.select(Graphics::hpAircraftBorder);
  canvas.polygon(Aircraft, n);
}

void
MapWindow::DrawGPSStatus(Canvas &canvas, const RECT &rc,
                         const GPS_STATE &gps) const
{
  const TCHAR *txt;
  MaskedIcon *icon = NULL;

  if (!gps.Connected) {
    icon = &Graphics::hGPSStatus2;
    txt = _("GPS not connected");
  } else if (gps.NAVWarning || (gps.SatellitesUsed == 0)) {
    icon = &Graphics::hGPSStatus1;
    txt = _("GPS waiting for fix");
  } else {
    return; // early exit
  }

  icon->draw(canvas, bitmap_canvas,
             rc.left + IBLSCALE(2),
            rc.bottom + IBLSCALE(-35));

  canvas.background_opaque();
  canvas.set_background_color(Color::WHITE);
  canvas.set_text_color(Color::BLACK);
  canvas.text(rc.left + IBLSCALE(24), rc.bottom + IBLSCALE(-32),
              txt);
}

void
MapWindow::DrawFlightMode(Canvas &canvas, const RECT &rc) const
{
  static bool flip = true;
  static fixed LastTime = fixed_zero;
  bool drawlogger = true;
  static bool lastLoggerActive = false;
  int offset = -1;

  // has GPS time advanced?
  if (Basic().Time <= LastTime) {
    LastTime = Basic().Time;
  } else {
    flip = !flip;

    // don't bother drawing logger if not active for more than one second
    if ((!logger.isLoggerActive()) && (!lastLoggerActive))
      drawlogger = false;

    lastLoggerActive = logger.isLoggerActive();
  }

  if (drawlogger) {
    offset -= 7;
    MaskedIcon &icon = (logger.isLoggerActive() && flip) ?
                       Graphics::hLogger : Graphics::hLoggerOff;

    icon.draw(canvas, bitmap_canvas,
              rc.right + IBLSCALE(offset),
              rc.bottom + IBLSCALE(-7));
  }

  MaskedIcon *bmp;

  if (task != NULL && (task->get_mode() == TaskManager::MODE_ABORT))
    bmp = &Graphics::hAbort;
  else if (render_projection.GetDisplayMode() == dmCircling)
    bmp = &Graphics::hClimb;
  else if (render_projection.GetDisplayMode() == dmFinalGlide)
    bmp = &Graphics::hFinalGlide;
  else
    bmp = &Graphics::hCruise;

  offset -= 24;

  bmp->draw(canvas, bitmap_canvas,
            rc.right + IBLSCALE(offset - 1),
            rc.bottom + IBLSCALE(-21));
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
                     wind.bearing - render_projection.GetDisplayAngle());

  canvas.polygon(Arrow, 5);

  if (SettingsMap().WindArrowStyle == 1) {
    POINT Tail[2] = {
      { 0, Layout::FastScale(-20) },
      { 0, Layout::FastScale(-26 - min(20, wmag) * 3) },
    };

    Angle angle = (wind.bearing - render_projection.GetDisplayAngle()).as_bearing();
    PolygonRotateShift(Tail, 2, Start.x, Start.y, angle);

    // optionally draw dashed line
    Pen dash_pen(Pen::DASH, 1, Color::BLACK);
    canvas.select(dash_pen);
    canvas.line(Tail[0], Tail[1]);
  }

  _stprintf(sTmp, _T("%i"),
            iround(Units::ToUserUnit(wind.norm, Units::WindSpeedUnit)));

  canvas.set_text_color(Color::BLACK);

  TextInBoxMode_t TextInBoxMode = { 16 | 32 };

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
MapWindow::DrawFinalGlide(Canvas &canvas, const RECT &rc) const
{
  POINT GlideBar[6] = {
      { 0, 0 }, { 9, -9 }, { 18, 0 }, { 18, 0 }, { 9, 0 }, { 0, 0 }
  };
  POINT GlideBar0[6] = {
      { 0, 0 }, { 9, -9 }, { 18, 0 }, { 18, 0 }, { 9, 0 }, { 0, 0 }
  };

  TCHAR Value[10];

  int Offset;
  int Offset0;
  int i;

  if (Calculated().task_stats.task_valid) {
    const int y0 = ((rc.bottom - rc.top) / 2) + rc.top;

    // 60 units is size, div by 8 means 60*8 = 480 meters.
    Offset = ((int)Calculated().task_stats.total.solution_remaining.AltitudeDifference) / 8;

    // JMW OLD_TASK this is broken now
    Offset0 = ((int)Calculated().task_stats.total.solution_mc0.AltitudeDifference) / 8;
    // TODO feature: should be an angle if in final glide mode

    if (Offset > 60)
      Offset = 60;
    if (Offset < -60)
      Offset = -60;

    Offset = IBLSCALE(Offset);
    if (Offset < 0)
      GlideBar[1].y = IBLSCALE(9);

    if (Offset0 > 60)
      Offset0 = 60;
    if (Offset0 < -60)
      Offset0 = -60;

    Offset0 = IBLSCALE(Offset0);
    if (Offset0 < 0)
      GlideBar0[1].y = IBLSCALE(9);

    for (i = 0; i < 6; i++) {
      GlideBar[i].y += y0;
      GlideBar[i].x = IBLSCALE(GlideBar[i].x) + rc.left;
    }

    GlideBar[0].y -= Offset;
    GlideBar[1].y -= Offset;
    GlideBar[2].y -= Offset;

    for (i = 0; i < 6; i++) {
      GlideBar0[i].y += y0;
      GlideBar0[i].x = IBLSCALE(GlideBar0[i].x) + rc.left;
    }

    GlideBar0[0].y -= Offset0;
    GlideBar0[1].y -= Offset0;
    GlideBar0[2].y -= Offset0;

    if ((Offset < 0) && (Offset0 < 0)) {
      // both below
      if (Offset0 != Offset) {
        int dy = (GlideBar0[0].y - GlideBar[0].y) +
            (GlideBar0[0].y - GlideBar0[3].y);
        dy = max(IBLSCALE(3), dy);
        GlideBar[3].y = GlideBar0[0].y - dy;
        GlideBar[4].y = GlideBar0[1].y - dy;
        GlideBar[5].y = GlideBar0[2].y - dy;

        GlideBar0[0].y = GlideBar[3].y;
        GlideBar0[1].y = GlideBar[4].y;
        GlideBar0[2].y = GlideBar[5].y;
      } else {
        Offset0 = 0;
      }
    } else if ((Offset > 0) && (Offset0 > 0)) {
      // both above
      GlideBar0[3].y = GlideBar[0].y;
      GlideBar0[4].y = GlideBar[1].y;
      GlideBar0[5].y = GlideBar[2].y;

      if (abs(Offset0 - Offset) < IBLSCALE(4))
        Offset = Offset0;
    }

    // draw actual glide bar
    if (Offset <= 0) {
      if (Calculated().common_stats.landable_reachable) {
        canvas.select(Graphics::hpFinalGlideBelowLandable);
        canvas.select(Graphics::hbFinalGlideBelowLandable);
      } else {
        canvas.select(Graphics::hpFinalGlideBelow);
        canvas.select(Graphics::hbFinalGlideBelow);
      }
    } else {
      canvas.select(Graphics::hpFinalGlideAbove);
      canvas.select(Graphics::hbFinalGlideAbove);
    }
    canvas.polygon(GlideBar, 6);

    // draw glide bar at mc 0
    if (Offset0 <= 0) {
      if (Calculated().common_stats.landable_reachable) {
        canvas.select(Graphics::hpFinalGlideBelowLandable);
        canvas.hollow_brush();
      } else {
        canvas.select(Graphics::hpFinalGlideBelow);
        canvas.hollow_brush();
      }
    } else {
      canvas.select(Graphics::hpFinalGlideAbove);
      canvas.hollow_brush();
    }

    if (Offset != Offset0)
      canvas.polygon(GlideBar0, 6);

    // draw x on final glide bar if unreachable at current Mc
    if (!Calculated().task_stats.total.achievable()) {
      canvas.select(Graphics::hpAircraftBorder);
      canvas.line(Layout::Scale(9 - 5), y0 + Layout::Scale(9 - 5),
                  Layout::Scale(9 + 5), y0 + Layout::Scale(9 + 5));
      canvas.line(Layout::Scale(9 - 5), y0 + Layout::Scale(9 + 5),
                  Layout::Scale(9 + 5), y0 + Layout::Scale(9 - 5));
    }

    if (Appearance.IndFinalGlide == fgFinalGlideDefault) {
      Units::FormatUserAltitude(Calculated().task_stats.total.solution_remaining.AltitudeDifference,
                                Value, sizeof(Value) / sizeof(Value[0]),
                                false);

      if (Offset >= 0)
        Offset = GlideBar[2].y + Offset + IBLSCALE(5);
      else if (Offset0 > 0)
        Offset = GlideBar0[1].y - IBLSCALE(15);
      else
        Offset = GlideBar[2].y + Offset - IBLSCALE(15);

      canvas.set_text_color(Color::BLACK);

      TextInBoxMode_t TextInBoxMode = { 1 | 8 };
      TextInBox(canvas, Value, 0, (int)Offset, TextInBoxMode, rc);

    } else if (Appearance.IndFinalGlide == fgFinalGlideAltA) {

      SIZE TextSize;
      int y = GlideBar[3].y;
      // was ((rc.bottom - rc.top )/2)-rc.top-
      //            Appearance.MapWindowBoldFont.CapitalHeight/2-1;
      int x = GlideBar[2].x + IBLSCALE(1);

      Units::FormatUserAltitude(Calculated().task_stats.total.solution_remaining.AltitudeDifference,
                                Value, sizeof(Value) / sizeof(Value[0]), false);

      canvas.select(Fonts::MapBold);
      TextSize = canvas.text_size(Value);

      canvas.fill_rectangle(x, y, x + IBLSCALE(1) + TextSize.cx,
                            y + Fonts::MapBold.get_capital_height() + IBLSCALE(2),
                            Color::WHITE);

      canvas.set_text_color(Color::BLACK);
      canvas.text(x + IBLSCALE(1),
                  y + Fonts::MapBold.get_capital_height() -
                  Fonts::MapBold.get_ascent_height() + IBLSCALE(1), Value);

      const UnitSymbol *unit_symbol = GetUnitSymbol(
        Units::GetUserAltitudeUnit());

      if (unit_symbol != NULL)
        unit_symbol->draw(canvas, bitmap_canvas,
                          x + TextSize.cx + IBLSCALE(1), y);
    }
  }
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
                       Angle::native(fixed_zero) - render_projection.GetDisplayAngle());
    canvas.polygon(Arrow, 5);
  } else if (Appearance.CompassAppearance == apCompassAltA) {

    static Angle lastDisplayAngle = Angle::native(fixed_zero);
    static int lastRcRight = 0;
    static POINT Arrow[5] = { { 0, -11 }, { -5, 9 }, { 0, 3 }, { 5, 9 }, { 0, -11 } };

    if (lastDisplayAngle != render_projection.GetDisplayAngle() ||
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
                         Angle::native(fixed_zero) - render_projection.GetDisplayAngle());

      lastDisplayAngle = render_projection.GetDisplayAngle();
      lastRcRight = rc.right;
    }
    canvas.polygon(Arrow, 5);

    canvas.select(Graphics::hpCompass);
    canvas.polygon(Arrow, 5);
  }
}

void
MapWindow::DrawBestCruiseTrack(Canvas &canvas) const
{
  if (!Calculated().task_stats.task_valid ||
      Calculated().task_stats.current_leg.solution_remaining.Vector.Distance
      < fixed(0.010))
    return;

  canvas.select(Graphics::hpBestCruiseTrack);
  canvas.select(Graphics::hbBestCruiseTrack);

  const Angle angle = Calculated().task_stats.current_leg.solution_remaining.CruiseTrackBearing
                    - render_projection.GetDisplayAngle();

  POINT Arrow[] = { { -1, -40 }, { -1, -62 }, { -6, -62 }, {  0, -70 },
                    {  6, -62 }, {  1, -62 }, {  1, -40 }, { -1, -40 } };

  PolygonRotateShift(Arrow, sizeof(Arrow) / sizeof(Arrow[0]),
                     render_projection.GetOrigAircraft().x,
                     render_projection.GetOrigAircraft().y,
                     angle);

  canvas.polygon(Arrow, sizeof(Arrow) / sizeof(Arrow[0]));
}

#include "Gauge/GaugeCDI.hpp"

void MapWindow::DrawCDI() {
  if (Calculated().Circling ?
      !SettingsMap().EnableCDICircling :
      !SettingsMap().EnableCDICruise) {
    cdi->hide_async();
    return;
  }

  cdi->update_async(Basic().TrackBearing,
                    Calculated().task_stats.current_leg.solution_remaining.Vector.Bearing);
}
