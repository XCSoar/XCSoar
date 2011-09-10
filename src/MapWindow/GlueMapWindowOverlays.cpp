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

#include "GlueMapWindow.hpp"
#include "Look/TaskLook.hpp"
#include "Screen/Icon.hpp"
#include "Language/Language.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Logger/Logger.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Screen/TextInBox.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Units/UnitsFormatter.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "UIState.hpp"
#include "Interface.hpp"
#include "Util/Macros.hpp"

#include <stdio.h>

void
GlueMapWindow::DrawCrossHairs(Canvas &canvas) const
{
  Pen dash_pen(Pen::DASH, 1, COLOR_DARK_GRAY);
  canvas.select(dash_pen);

  const RasterPoint Orig_Screen = render_projection.GetScreenOrigin();

  canvas.line(Orig_Screen.x + 20, Orig_Screen.y,
              Orig_Screen.x - 20, Orig_Screen.y);
  canvas.line(Orig_Screen.x, Orig_Screen.y + 20,
              Orig_Screen.x, Orig_Screen.y - 20);
}

void
GlueMapWindow::DrawGPSStatus(Canvas &canvas, const PixelRect &rc,
                             const NMEAInfo &info) const
{
  const TCHAR *txt;
  MaskedIcon *icon = NULL;

  if (!info.connected) {
    icon = &Graphics::hGPSStatus2;
    txt = _("GPS not connected");
  } else if (!info.location_available) {
    icon = &Graphics::hGPSStatus1;
    txt = _("GPS waiting for fix");
  } else {
    return; // early exit
  }

  int x = rc.left + Layout::FastScale(2);
  int y = rc.bottom - Layout::FastScale(35);
  icon->draw(canvas, x, y);

  x += icon->get_size().cx + Layout::FastScale(4);
  y = rc.bottom - Layout::FastScale(34);

  TextInBoxMode_t mode;
  mode.Mode = RoundedBlack;
  mode.Bold = true;

  TextInBox(canvas, txt, x, y, mode, rc, NULL);
}

void
GlueMapWindow::DrawFlightMode(Canvas &canvas, const PixelRect &rc) const
{
  int offset = 0;

  // draw logger status
  if (logger != NULL && logger->isLoggerActive()) {
    bool flip = (Basic().date_time_utc.second % 2) == 0;
    MaskedIcon &icon = flip ? Graphics::hLogger : Graphics::hLoggerOff;
    offset = icon.get_size().cx;
    icon.draw(canvas, rc.right - offset, rc.bottom - icon.get_size().cy);
  }

  // draw flight mode
  MaskedIcon *bmp;

  if (task != NULL && (task->get_mode() == TaskManager::MODE_ABORT))
    bmp = &Graphics::hAbort;
  else if (GetDisplayMode() == DM_CIRCLING)
    bmp = &Graphics::hClimb;
  else if (GetDisplayMode() == DM_FINAL_GLIDE)
    bmp = &Graphics::hFinalGlide;
  else
    bmp = &Graphics::hCruise;

  offset += bmp->get_size().cx + Layout::Scale(6);

  bmp->draw(canvas, rc.right - offset,
            rc.bottom - bmp->get_size().cy - Layout::Scale(4));

  // draw flarm status
  if (CommonInterface::GetUISettings().enable_flarm_gauge)
    // Don't show indicator when the gauge is indicating the traffic anyway
    return;

  const FLARM_STATE &flarm = Basic().flarm;
  if (!flarm.available || (flarm.GetActiveTrafficCount()==0))
    return;
  switch (flarm.alarm_level) {
  case 0:
    bmp = &Graphics::hBmpTrafficSafe;
    break;
  case 1:
    bmp = &Graphics::hBmpTrafficWarning;
    break;
  case 2:
  case 3:
    bmp = &Graphics::hBmpTrafficAlarm;
    break;
  };

  offset += bmp->get_size().cx + Layout::Scale(6);

  bmp->draw(canvas, rc.right - offset,
            rc.bottom - bmp->get_size().cy - Layout::Scale(2));
}

void
GlueMapWindow::DrawFinalGlide(Canvas &canvas, const PixelRect &rc) const
{
  RasterPoint GlideBar[6] = {
      { 0, 0 }, { 9, -9 }, { 18, 0 }, { 18, 0 }, { 9, 0 }, { 0, 0 }
  };
  RasterPoint GlideBar0[6] = {
      { 0, 0 }, { 9, -9 }, { 18, 0 }, { 18, 0 }, { 9, 0 }, { 0, 0 }
  };

  TCHAR Value[10];

  int Offset;
  int Offset0;
  int i;

  if (!Calculated().task_stats.task_valid ||
      !Calculated().task_stats.total.solution_remaining.IsOk() ||
      !Calculated().task_stats.total.solution_mc0.IsDefined())
    return;

  const int y0 = ((rc.bottom - rc.top) / 2) + rc.top;

  // 60 units is size, div by 8 means 60*8 = 480 meters.
  Offset = ((int)Calculated().task_stats.total.solution_remaining.altitude_difference) / 8;
  Offset0 = ((int)Calculated().task_stats.total.solution_mc0.altitude_difference) / 8;
  // TODO feature: should be an angle if in final glide mode

  if (Offset > 60)
    Offset = 60;
  if (Offset < -60)
    Offset = -60;

  Offset = Layout::Scale(Offset);
  if (Offset < 0)
    GlideBar[1].y = Layout::Scale(9);

  if (Offset0 > 60)
    Offset0 = 60;
  if (Offset0 < -60)
    Offset0 = -60;

  Offset0 = Layout::Scale(Offset0);
  if (Offset0 < 0)
    GlideBar0[1].y = Layout::Scale(9);

  for (i = 0; i < 6; i++) {
    GlideBar[i].y += y0;
    GlideBar[i].x = Layout::Scale(GlideBar[i].x) + rc.left;
  }

  GlideBar[0].y -= Offset;
  GlideBar[1].y -= Offset;
  GlideBar[2].y -= Offset;

  for (i = 0; i < 6; i++) {
    GlideBar0[i].y += y0;
    GlideBar0[i].x = Layout::Scale(GlideBar0[i].x) + rc.left;
  }

  GlideBar0[0].y -= Offset0;
  GlideBar0[1].y -= Offset0;
  GlideBar0[2].y -= Offset0;

  if ((Offset < 0) && (Offset0 < 0)) {
    // both below
    if (Offset0 != Offset) {
      int dy = (GlideBar0[0].y - GlideBar[0].y) +
          (GlideBar0[0].y - GlideBar0[3].y);
      dy = max(Layout::Scale(3), dy);
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

    if (abs(Offset0 - Offset) < Layout::Scale(4))
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

  // draw cross (x) on final glide bar if unreachable at current Mc
  // or above final glide but impeded by obstacle
  int cross_sign = 0;

  if (!Calculated().task_stats.total.IsAchievable())
    cross_sign = 1;
  if (Calculated().terrain_warning && (Offset>0))
    cross_sign = -1;

  if (cross_sign != 0) {
    canvas.select(task_look.bearing_pen);
    canvas.line(Layout::Scale(9 - 5), y0 + cross_sign * Layout::Scale(9 - 5),
                Layout::Scale(9 + 5), y0 + cross_sign * Layout::Scale(9 + 5));
    canvas.line(Layout::Scale(9 - 5), y0 + cross_sign * Layout::Scale(9 + 5),
                Layout::Scale(9 + 5), y0 + cross_sign * Layout::Scale(9 - 5));
  }

  Units::FormatUserAltitude(Calculated().task_stats.total.solution_remaining.altitude_difference,
                            Value, ARRAY_SIZE(Value),
                            false);

  if (Offset >= 0)
    Offset = GlideBar[2].y + Offset + Layout::Scale(5);
  else if (Offset0 > 0)
    Offset = GlideBar0[1].y - Layout::Scale(15);
  else
    Offset = GlideBar[2].y + Offset - Layout::Scale(15);

  canvas.set_text_color(COLOR_BLACK);
  canvas.set_background_color(COLOR_WHITE);

  TextInBoxMode_t TextInBoxMode;
  TextInBoxMode.Mode = RoundedBlack;
  TextInBoxMode.Bold = true;
  TextInBoxMode.MoveInView = true;
  TextInBox(canvas, Value, 0, (int)Offset, TextInBoxMode, rc);
}

void
GlueMapWindow::DrawMapScale(Canvas &canvas, const PixelRect &rc,
                            const MapWindowProjection &projection) const
{
  fixed MapWidth;
  TCHAR ScaleInfo[80];

  Units_t Unit;

  MapWidth = projection.GetScreenWidthMeters();

  canvas.select(Fonts::MapBold);
  Units::FormatUserMapScale(&Unit, MapWidth, ScaleInfo,
                            sizeof(ScaleInfo) / sizeof(TCHAR), true);
  PixelSize TextSize = canvas.text_size(ScaleInfo);

  int Height = Fonts::MapBold.get_capital_height() + Layout::Scale(2);
  // 2: add 1pix border

  canvas.fill_rectangle(Layout::Scale(4), rc.bottom - Height,
                        TextSize.cx + Layout::Scale(11), rc.bottom,
                        COLOR_WHITE);

  canvas.background_transparent();
  canvas.set_text_color(COLOR_BLACK);

  canvas.text(Layout::Scale(7),
              rc.bottom - Fonts::MapBold.get_ascent_height() - Layout::Scale(1),
              ScaleInfo);

  Graphics::hBmpMapScaleLeft.draw(canvas, 0, rc.bottom - Height);
  Graphics::hBmpMapScaleRight.draw(canvas, Layout::Scale(9) + TextSize.cx,
                                   rc.bottom - Height);

  ScaleInfo[0] = '\0';
  if (SettingsMap().AutoZoom)
    _tcscat(ScaleInfo, _T("AUTO "));

  switch (follow_mode) {
  case FOLLOW_SELF:
    break;

  case FOLLOW_PAN:
    _tcscat(ScaleInfo, _T("PAN "));
    break;
  }

  const UIState &ui_state = CommonInterface::GetUIState();
  if (ui_state.auxiliary_enabled) {
    _tcscat(ScaleInfo, InfoBoxManager::GetCurrentPanelName());
    _tcscat(ScaleInfo, _T(" "));
  }

  if (Basic().gps.replay)
    _tcscat(ScaleInfo, _T("REPLAY "));
  else if (Basic().gps.simulator) {
    _tcscat(ScaleInfo, _("Simulator"));
    _tcscat(ScaleInfo, _T(" "));
  }

  if (SettingsComputer().BallastTimerActive) {
    TCHAR TEMP[20];
    _stprintf(TEMP, _T("BALLAST %d LITERS "),
              (int)SettingsComputer().glide_polar_task.GetBallastLitres());
    _tcscat(ScaleInfo, TEMP);
  }

  if (weather != NULL && weather->GetParameter() > 0) {
    const TCHAR *label = weather->ItemLabel(weather->GetParameter());
    if (label != NULL)
      _tcscat(ScaleInfo, label);
  }

  if (ScaleInfo[0]) {
    int y = rc.bottom - Height;

    canvas.select(Fonts::Title);
    canvas.background_opaque();
    canvas.set_background_color(COLOR_WHITE);

    TextSize = canvas.text_size(ScaleInfo);
    y-= TextSize.cy;
    canvas.text(0, y, ScaleInfo);
  }
}

void
GlueMapWindow::DrawThermalEstimate(Canvas &canvas) const
{
  if (GetDisplayMode() == DM_CIRCLING) {
    // in circling mode, draw thermal at actual estimated location
    const MapWindowProjection &projection = render_projection;
    const ThermalLocatorInfo &thermal_locator = Calculated().thermal_locator;
    if (thermal_locator.estimate_valid) {
      RasterPoint sc;
      if (projection.GeoToScreenIfVisible(thermal_locator.estimate_location, sc)) {
        Graphics::hBmpThermalSource.draw(canvas, sc);
      }
    }
  } else {
    MapWindow::DrawThermalEstimate(canvas);
  }
}

void
GlueMapWindow::RenderTrail(Canvas &canvas, const RasterPoint aircraft_pos) const
{
  unsigned min_time = 0;
  if (GetDisplayMode() == DM_CIRCLING) {
    min_time = max(0, (int)Basic().time - 600);
  } else {
    switch(SettingsMap().trail_length) {
    case TRAIL_OFF:
      return;
    case TRAIL_LONG:
      min_time = max(0, (int)Basic().time - 3600);
      break;
    case TRAIL_SHORT:
      min_time = max(0, (int)Basic().time - 600);
      break;
    case TRAIL_FULL:
      min_time = 0; // full
      break;
    }
  }

  DrawTrail(canvas, aircraft_pos, min_time,
            SettingsMap().EnableTrailDrift && GetDisplayMode() == DM_CIRCLING);
}

void
GlueMapWindow::DrawThermalBand(Canvas &canvas, const PixelRect &rc) const
{
  if (Calculated().task_stats.total.solution_remaining.IsOk() &&
      Calculated().task_stats.total.solution_remaining.altitude_difference > fixed(50)
      && GetDisplayMode() == DM_FINAL_GLIDE)
    return;

  PixelRect tb_rect;
  tb_rect.left = rc.left;
  tb_rect.right = rc.left+Layout::Scale(20);
  tb_rect.top = Layout::Scale(4);
  tb_rect.bottom = tb_rect.top+(rc.bottom-rc.top)/2-Layout::Scale(30);

  const ThermalBandRenderer &renderer = thermal_band_renderer;
  if (task != NULL) {
    ProtectedTaskManager::Lease task_manager(*task);
    renderer.DrawThermalBand(Basic(),
                             Calculated(),
                             SettingsComputer(),
                             canvas,
                             tb_rect,
                             SettingsComputer().task,
                             true,
                             &task_manager->get_ordered_task_behaviour());
  } else {
    renderer.DrawThermalBand(Basic(),
                             Calculated(),
                             SettingsComputer(),
                             canvas,
                             tb_rect,
                             SettingsComputer().task,
                             true);
  }
}

void
GlueMapWindow::DrawStallRatio(Canvas &canvas, const PixelRect &rc) const
{
  if (Basic().stall_ratio_available) {
    // JMW experimental, display stall sensor
    fixed s = max(fixed_zero, min(fixed_one, Basic().stall_ratio));
    long m = (long)((rc.bottom - rc.top) * s * s);

    canvas.black_pen();
    canvas.line(rc.right - 1, rc.bottom - m, rc.right - 11, rc.bottom - m);
  }
}
