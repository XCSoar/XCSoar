/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Look/MapLook.hpp"
#include "Look/TaskLook.hpp"
#include "Look/Fonts.hpp"
#include "Screen/Icon.hpp"
#include "Language/Language.hpp"
#include "Screen/Layout.hpp"
#include "Logger/Logger.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Renderer/TextInBox.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "UIState.hpp"
#include "Renderer/FinalGlideBarRenderer.hpp"
#include "Units/Units.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Util/Macros.hpp"
#include "Look/GestureLook.hpp"
#include "Input/InputEvents.hpp"

#include <stdio.h>

void
GlueMapWindow::DrawGesture(Canvas &canvas) const
{
  if (!gestures.HasPoints())
    return;

  const TCHAR *gesture = gestures.GetGesture();
  if (gesture != NULL && !InputEvents::IsGesture(gesture))
    canvas.Select(gesture_look.invalid_pen);
  else
    canvas.Select(gesture_look.pen);

  canvas.SelectHollowBrush();

  const auto &points = gestures.GetPoints();
  auto it = points.begin();
  auto it_last = it++;
  for (auto it_end = points.end(); it != it_end; it_last = it++)
    canvas.DrawLinePiece(*it_last, *it);
}

void
GlueMapWindow::DrawCrossHairs(Canvas &canvas) const
{
  Pen dash_pen(Pen::DASH, 1, COLOR_DARK_GRAY);
  canvas.Select(dash_pen);

  const RasterPoint center = render_projection.GetScreenOrigin();

  canvas.DrawLine(center.x + 20, center.y,
              center.x - 20, center.y);
  canvas.DrawLine(center.x, center.y + 20,
              center.x, center.y - 20);
}

void
GlueMapWindow::DrawPanInfo(Canvas &canvas) const
{
  GeoPoint location = render_projection.GetGeoLocation();

  TextInBoxMode mode;
  mode.shape = LabelShape::OUTLINED_INVERTED;
  mode.align = TextInBoxMode::Alignment::RIGHT;

  canvas.Select(Fonts::map_bold);

  UPixelScalar padding = Layout::FastScale(4);
  UPixelScalar height = Fonts::map_bold.GetHeight();
  PixelScalar y = 0 + padding;
  PixelScalar x = render_projection.GetScreenWidth() - padding;

  if (terrain) {
    short elevation = terrain->GetTerrainHeight(location);
    if (!RasterBuffer::IsSpecial(elevation)) {
      StaticString<64> elevation_short, elevation_long;
      FormatUserAltitude(fixed(elevation),
                                elevation_short.buffer(), elevation_short.MAX_SIZE);

      elevation_long = _("Elevation: ");
      elevation_long += elevation_short;

      TextInBox(canvas, elevation_long, x, y, mode,
                render_projection.GetScreenWidth(),
                render_projection.GetScreenHeight());

      y += height;
    }
  }

  TCHAR buffer[256];
  FormatGeoPoint(location, buffer, ARRAY_SIZE(buffer), _T('\n'));

  TCHAR *start = buffer;
  while (true) {
    TCHAR *newline = _tcschr(start, _T('\n'));
    if (newline != NULL)
      *newline = _T('\0');

    TextInBox(canvas, start, x, y, mode,
              render_projection.GetScreenWidth(),
              render_projection.GetScreenHeight());

    y += height;

    if (newline == NULL)
      break;

    start = newline + 1;
  }
}

void
GlueMapWindow::DrawGPSStatus(Canvas &canvas, const PixelRect &rc,
                             const NMEAInfo &info) const
{
  const TCHAR *txt;
  const MaskedIcon *icon;

  if (!info.alive) {
    icon = &look.no_gps_icon;
    txt = _("GPS not connected");
  } else if (!info.location_available) {
    icon = &look.waiting_for_fix_icon;
    txt = _("GPS waiting for fix");
  } else
    // early exit
    return;

  PixelScalar x = rc.left + Layout::FastScale(2);
  PixelScalar y = rc.bottom - Layout::FastScale(35);
  icon->Draw(canvas, x, y);

  x += icon->GetSize().cx + Layout::FastScale(4);
  y = rc.bottom - Layout::FastScale(34);

  TextInBoxMode mode;
  mode.shape = LabelShape::ROUNDED_BLACK;

  canvas.Select(Fonts::map_bold);
  TextInBox(canvas, txt, x, y, mode, rc, NULL);
}

void
GlueMapWindow::DrawFlightMode(Canvas &canvas, const PixelRect &rc) const
{
  PixelScalar offset = 0;

  // draw logger status
  if (logger != NULL && logger->IsLoggerActive()) {
    bool flip = (Basic().date_time_utc.second % 2) == 0;
    const MaskedIcon &icon = flip ? look.logger_on_icon : look.logger_off_icon;
    offset = icon.GetSize().cx;
    icon.Draw(canvas, rc.right - offset, rc.bottom - icon.GetSize().cy);
  }

  // draw flight mode
  const MaskedIcon *bmp;

  if (Calculated().common_stats.mode_abort)
    bmp = &look.abort_mode_icon;
  else if (GetDisplayMode() == DisplayMode::CIRCLING)
    bmp = &look.climb_mode_icon;
  else if (GetDisplayMode() == DisplayMode::FINAL_GLIDE)
    bmp = &look.final_glide_mode_icon;
  else
    bmp = &look.cruise_mode_icon;

  offset += bmp->GetSize().cx + Layout::Scale(6);

  bmp->Draw(canvas, rc.right - offset,
            rc.bottom - bmp->GetSize().cy - Layout::Scale(4));

  // draw flarm status
  if (!GetMapSettings().show_flarm_alarm_level)
    // Don't show indicator when the gauge is indicating the traffic anyway
    return;

  const FlarmStatus &flarm = Basic().flarm.status;
  if (!flarm.available)
    return;

  switch (flarm.alarm_level) {
  case FlarmTraffic::AlarmType::NONE:
    bmp = &look.traffic_safe_icon;
    break;
  case FlarmTraffic::AlarmType::LOW:
  case FlarmTraffic::AlarmType::INFO_ALERT:
    bmp = &look.traffic_warning_icon;
    break;
  case FlarmTraffic::AlarmType::IMPORTANT:
  case FlarmTraffic::AlarmType::URGENT:
    bmp = &look.traffic_alarm_icon;
    break;
  };

  offset += bmp->GetSize().cx + Layout::Scale(6);

  bmp->Draw(canvas, rc.right - offset,
            rc.bottom - bmp->GetSize().cy - Layout::Scale(2));
}

void
GlueMapWindow::DrawFinalGlide(Canvas &canvas, const PixelRect &rc) const
{
  final_glide_bar_renderer.Draw(canvas, rc, Calculated(),
                                GetComputerSettings().task.glide,
                                GetMapSettings().final_glide_bar_mc0_enabled);
}

void
GlueMapWindow::DrawMapScale(Canvas &canvas, const PixelRect &rc,
                            const MapWindowProjection &projection) const
{
  StaticString<80> buffer;

  fixed map_width = projection.GetScreenWidthMeters();

  canvas.Select(Fonts::map_bold);
  FormatUserMapScale(map_width, buffer.buffer(), true);
  PixelSize text_size = canvas.CalcTextSize(buffer);

  const PixelScalar text_padding_x = Layout::GetTextPadding();
  const PixelScalar height = Fonts::map_bold.GetCapitalHeight()
    + Layout::GetTextPadding();

  PixelScalar x = 0;
  look.map_scale_left_icon.Draw(canvas, 0, rc.bottom - height);

  x += look.map_scale_left_icon.GetSize().cx;
  canvas.DrawFilledRectangle(x, rc.bottom - height,
                             x + 2 * text_padding_x + text_size.cx,
                             rc.bottom, COLOR_WHITE);

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);
  x += text_padding_x;
  canvas.DrawText(x,
                  rc.bottom - Fonts::map_bold.GetAscentHeight() - Layout::Scale(1),
                  buffer);

  x += text_padding_x + text_size.cx;
  look.map_scale_right_icon.Draw(canvas, x, rc.bottom - height);

  buffer.clear();
  if (GetMapSettings().auto_zoom_enabled)
    buffer = _T("AUTO ");

  switch (follow_mode) {
  case FOLLOW_SELF:
    break;

  case FOLLOW_PAN:
    buffer += _T("PAN ");
    break;
  }

  const UIState &ui_state = GetUIState();
  if (ui_state.auxiliary_enabled) {
    buffer += ui_state.panel_name;
    buffer += _T(" ");
  }

  if (Basic().gps.replay)
    buffer += _T("REPLAY ");
  else if (Basic().gps.simulator) {
    buffer += _("Simulator");
    buffer += _T(" ");
  }

  if (GetComputerSettings().polar.ballast_timer_active)
    buffer.AppendFormat(
        _T("BALLAST %d LITERS "),
        (int)GetComputerSettings().polar.glide_polar_task.GetBallastLitres());

  if (weather != NULL && weather->GetParameter() > 0) {
    const TCHAR *label = weather->ItemLabel(weather->GetParameter());
    if (label != NULL)
      buffer += label;
  }

  if (!buffer.empty()) {
    int y = rc.bottom - height;

    canvas.Select(Fonts::title);
    canvas.SetBackgroundOpaque();
    canvas.SetBackgroundColor(COLOR_WHITE);

    canvas.DrawText(0, y - canvas.CalcTextSize(buffer).cy, buffer);
  }
}

void
GlueMapWindow::DrawThermalEstimate(Canvas &canvas) const
{
  if (InCirclingMode()) {
    // in circling mode, draw thermal at actual estimated location
    const MapWindowProjection &projection = render_projection;
    const ThermalLocatorInfo &thermal_locator = Calculated().thermal_locator;
    if (thermal_locator.estimate_valid) {
      RasterPoint sc;
      if (projection.GeoToScreenIfVisible(thermal_locator.estimate_location, sc)) {
        look.thermal_source_icon.Draw(canvas, sc);
      }
    }
  } else {
    MapWindow::DrawThermalEstimate(canvas);
  }
}

void
GlueMapWindow::RenderTrail(Canvas &canvas, const RasterPoint aircraft_pos)
{
  unsigned min_time;
  switch(GetMapSettings().trail.length) {
  case TrailSettings::Length::OFF:
    return;
  case TrailSettings::Length::LONG:
    min_time = max(0, (int)Basic().time - 3600);
    break;
  case TrailSettings::Length::SHORT:
    min_time = max(0, (int)Basic().time - 600);
    break;
  case TrailSettings::Length::FULL:
    min_time = 0; // full
    break;
  }

  DrawTrail(canvas, aircraft_pos, min_time,
            GetMapSettings().trail.wind_drift_enabled && InCirclingMode());
}

void
GlueMapWindow::DrawThermalBand(Canvas &canvas, const PixelRect &rc) const
{
  if (Calculated().task_stats.total.solution_remaining.IsOk() &&
      Calculated().task_stats.total.solution_remaining.altitude_difference > fixed(50)
      && GetDisplayMode() == DisplayMode::FINAL_GLIDE)
    return;

  PixelRect tb_rect;
  tb_rect.left = rc.left;
  tb_rect.right = rc.left+Layout::Scale(20);
  tb_rect.top = Layout::Scale(2);
  tb_rect.bottom = (rc.bottom-rc.top)/2-Layout::Scale(73);

  const ThermalBandRenderer &renderer = thermal_band_renderer;
  if (task != NULL) {
    ProtectedTaskManager::Lease task_manager(*task);
    renderer.DrawThermalBand(Basic(),
                             Calculated(),
                             GetComputerSettings(),
                             canvas,
                             tb_rect,
                             GetComputerSettings().task,
                             true,
                             &task_manager->GetOrderedTaskBehaviour());
  } else {
    renderer.DrawThermalBand(Basic(),
                             Calculated(),
                             GetComputerSettings(),
                             canvas,
                             tb_rect,
                             GetComputerSettings().task,
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

    canvas.SelectBlackPen();
    canvas.DrawLine(rc.right - 1, rc.bottom - m, rc.right - 11, rc.bottom - m);
  }
}
