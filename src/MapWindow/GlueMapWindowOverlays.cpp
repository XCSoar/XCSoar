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
#include "Renderer/FinalGlideBarRenderer.hpp"

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

  PixelScalar x = rc.left + Layout::FastScale(2);
  PixelScalar y = rc.bottom - Layout::FastScale(35);
  icon->Draw(canvas, x, y);

  x += icon->GetSize().cx + Layout::FastScale(4);
  y = rc.bottom - Layout::FastScale(34);

  TextInBoxMode mode;
  mode.mode = RM_ROUNDED_BLACK;
  mode.bold = true;

  TextInBox(canvas, txt, x, y, mode, rc, NULL);
}

void
GlueMapWindow::DrawFlightMode(Canvas &canvas, const PixelRect &rc) const
{
  PixelScalar offset = 0;

  // draw logger status
  if (logger != NULL && logger->IsLoggerActive()) {
    bool flip = (Basic().date_time_utc.second % 2) == 0;
    MaskedIcon &icon = flip ? Graphics::hLogger : Graphics::hLoggerOff;
    offset = icon.GetSize().cx;
    icon.Draw(canvas, rc.right - offset, rc.bottom - icon.GetSize().cy);
  }

  // draw flight mode
  MaskedIcon *bmp;

  if (task != NULL && (task->GetMode() == TaskManager::MODE_ABORT))
    bmp = &Graphics::hAbort;
  else if (GetDisplayMode() == DM_CIRCLING)
    bmp = &Graphics::hClimb;
  else if (GetDisplayMode() == DM_FINAL_GLIDE)
    bmp = &Graphics::hFinalGlide;
  else
    bmp = &Graphics::hCruise;

  offset += bmp->GetSize().cx + Layout::Scale(6);

  bmp->Draw(canvas, rc.right - offset,
            rc.bottom - bmp->GetSize().cy - Layout::Scale(4));

  // draw flarm status
  if (CommonInterface::GetUISettings().enable_flarm_gauge)
    // Don't show indicator when the gauge is indicating the traffic anyway
    return;

  const FlarmState &flarm = Basic().flarm;
  if (!flarm.available || flarm.GetActiveTrafficCount() == 0)
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

  offset += bmp->GetSize().cx + Layout::Scale(6);

  bmp->Draw(canvas, rc.right - offset,
            rc.bottom - bmp->GetSize().cy - Layout::Scale(2));
}

void
GlueMapWindow::DrawFinalGlide(Canvas &canvas, const PixelRect &rc) const
{
  FinalGlideBarRenderer::Draw(canvas, rc, Calculated(), task_look);
}

void
GlueMapWindow::DrawMapScale(Canvas &canvas, const PixelRect &rc,
                            const MapWindowProjection &projection) const
{
  TCHAR buffer[80];

  fixed MapWidth = projection.GetScreenWidthMeters();

  canvas.select(Fonts::MapBold);
  Units::FormatUserMapScale(MapWidth, buffer,
                            sizeof(buffer) / sizeof(TCHAR), true);
  PixelSize TextSize = canvas.text_size(buffer);

  UPixelScalar Height = Fonts::MapBold.GetCapitalHeight() + Layout::Scale(2);
  // 2: add 1pix border

  canvas.fill_rectangle(Layout::Scale(4), rc.bottom - Height,
                        TextSize.cx + Layout::Scale(11), rc.bottom,
                        COLOR_WHITE);

  canvas.background_transparent();
  canvas.set_text_color(COLOR_BLACK);

  canvas.text(Layout::Scale(7),
              rc.bottom - Fonts::MapBold.GetAscentHeight() - Layout::Scale(1),
              buffer);

  Graphics::hBmpMapScaleLeft.Draw(canvas, 0, rc.bottom - Height);
  Graphics::hBmpMapScaleRight.Draw(canvas, Layout::Scale(9) + TextSize.cx,
                                   rc.bottom - Height);

  buffer[0] = '\0';
  if (SettingsMap().AutoZoom)
    _tcscat(buffer, _T("AUTO "));

  switch (follow_mode) {
  case FOLLOW_SELF:
    break;

  case FOLLOW_PAN:
    _tcscat(buffer, _T("PAN "));
    break;
  }

  const UIState &ui_state = CommonInterface::GetUIState();
  if (ui_state.auxiliary_enabled) {
    _tcscat(buffer, InfoBoxManager::GetCurrentPanelName());
    _tcscat(buffer, _T(" "));
  }

  if (Basic().gps.replay)
    _tcscat(buffer, _T("REPLAY "));
  else if (Basic().gps.simulator) {
    _tcscat(buffer, _("Simulator"));
    _tcscat(buffer, _T(" "));
  }

  if (SettingsComputer().BallastTimerActive) {
    TCHAR TEMP[20];
    _stprintf(TEMP, _T("BALLAST %d LITERS "),
              (int)SettingsComputer().glide_polar_task.GetBallastLitres());
    _tcscat(buffer, TEMP);
  }

  if (weather != NULL && weather->GetParameter() > 0) {
    const TCHAR *label = weather->ItemLabel(weather->GetParameter());
    if (label != NULL)
      _tcscat(buffer, label);
  }

  if (buffer[0]) {
    int y = rc.bottom - Height;

    canvas.select(Fonts::Title);
    canvas.background_opaque();
    canvas.set_background_color(COLOR_WHITE);

    canvas.text(0, y - canvas.text_size(buffer).cy, buffer);
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
        Graphics::hBmpThermalSource.Draw(canvas, sc);
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
                             &task_manager->GetOrderedTaskBehaviour());
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
    PixelScalar m((rc.bottom - rc.top) * s * s);

    canvas.black_pen();
    canvas.line(rc.right - 1, rc.bottom - m, rc.right - 11, rc.bottom - m);
  }
}
