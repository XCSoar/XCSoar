/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Screen/Icon.hpp"
#include "Language/Language.hpp"
#include "Screen/Layout.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Renderer/TextInBox.hpp"
#include "Weather/Rasp/RaspRenderer.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "UIState.hpp"
#include "Renderer/FinalGlideBarRenderer.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Util/Macros.hpp"
#include "Util/Clamp.hpp"
#include "Util/StringAPI.hxx"
#include "Look/GestureLook.hpp"
#include "Input/InputEvents.hpp"
#include "Renderer/MapScaleRenderer.hpp"

#include <stdio.h>

void
GlueMapWindow::DrawGesture(Canvas &canvas) const
{
  if (!gestures.HasPoints())
    return;

  const TCHAR *gesture = gestures.GetGesture();
  if (gesture != nullptr && !InputEvents::IsGesture(gesture))
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
  if (!render_projection.IsValid())
    return;

  canvas.Select(look.overlay.crosshair_pen);

  const auto center = render_projection.GetScreenOrigin();

  canvas.DrawLine(center.x + 20, center.y,
              center.x - 20, center.y);
  canvas.DrawLine(center.x, center.y + 20,
              center.x, center.y - 20);
}

void
GlueMapWindow::DrawPanInfo(Canvas &canvas) const
{
  if (!render_projection.IsValid())
    return;

  GeoPoint location = render_projection.GetGeoLocation();

  TextInBoxMode mode;
  mode.shape = LabelShape::OUTLINED;
  mode.align = TextInBoxMode::Alignment::RIGHT;

  const Font &font = *look.overlay.overlay_font;
  canvas.Select(font);

  unsigned padding = Layout::FastScale(4);
  unsigned height = font.GetHeight();
  int y = 0 + padding;
  int x = render_projection.GetScreenWidth() - padding;

  if (compass_visible)
    /* don't obscure the north arrow */
    /* TODO: obtain offset from CompassRenderer */
    y += Layout::Scale(19) + Layout::FastScale(13);

  if (terrain) {
    TerrainHeight elevation = terrain->GetTerrainHeight(location);
    if (!elevation.IsSpecial()) {
      StaticString<64> elevation_long;
      elevation_long = _("Elevation: ");
      elevation_long += FormatUserAltitude(elevation.GetValue());

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
    auto *newline = StringFind(start, _T('\n'));
    if (newline != nullptr)
      *newline = _T('\0');

    TextInBox(canvas, start, x, y, mode,
              render_projection.GetScreenWidth(),
              render_projection.GetScreenHeight());

    y += height;

    if (newline == nullptr)
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

  PixelPoint p(rc.left + Layout::FastScale(2),
               rc.bottom - Layout::FastScale(35));
  icon->Draw(canvas, p);

  p.x += icon->GetSize().cx + Layout::FastScale(4);
  p.y = rc.bottom - Layout::FastScale(34);

  TextInBoxMode mode;
  mode.shape = LabelShape::ROUNDED_BLACK;

  const Font &font = *look.overlay.overlay_font;
  canvas.Select(font);
  TextInBox(canvas, txt, p.x, p.y, mode, rc, nullptr);
}

void
GlueMapWindow::DrawFlightMode(Canvas &canvas, const PixelRect &rc) const
{
  int offset = 0;

  // draw flight mode
  const MaskedIcon *bmp;

  if (Calculated().common_stats.task_type == TaskType::ABORT)
    bmp = &look.abort_mode_icon;
  else if (GetDisplayMode() == DisplayMode::CIRCLING)
    bmp = &look.climb_mode_icon;
  else if (GetDisplayMode() == DisplayMode::FINAL_GLIDE)
    bmp = &look.final_glide_mode_icon;
  else
    bmp = &look.cruise_mode_icon;

  offset += bmp->GetSize().cx + Layout::Scale(6);

  bmp->Draw(canvas,
            PixelPoint(rc.right - offset,
                       rc.bottom - bmp->GetSize().cy - Layout::Scale(4)));

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

  bmp->Draw(canvas,
            PixelPoint(rc.right - offset,
                       rc.bottom - bmp->GetSize().cy - Layout::Scale(2)));
}

void
GlueMapWindow::DrawFinalGlide(Canvas &canvas, const PixelRect &rc) const
{

  if (GetMapSettings().final_glide_bar_display_mode==FinalGlideBarDisplayMode::OFF)
    return;

  if (GetMapSettings().final_glide_bar_display_mode==FinalGlideBarDisplayMode::AUTO) {
    const TaskStats &task_stats = Calculated().task_stats;
    const ElementStat &total = task_stats.total;
    const GlideResult &solution = total.solution_remaining;
    const GlideResult &solution_mc0 = total.solution_mc0;
    const GlideSettings &glide_settings= GetComputerSettings().task.glide;

    if (!task_stats.task_valid || !solution.IsOk() || !solution_mc0.IsDefined())
      return;

    if (solution_mc0.SelectAltitudeDifference(glide_settings) < -1000 &&
        solution.SelectAltitudeDifference(glide_settings) < -1000)
      return;
  }

  final_glide_bar_renderer.Draw(canvas, rc, Calculated(),
                                GetComputerSettings().task.glide,
                                GetMapSettings().final_glide_bar_mc0_enabled);
}

void
GlueMapWindow::DrawVario(Canvas &canvas, const PixelRect &rc) const
{
  if (!GetMapSettings().vario_bar_enabled)
   return;

  vario_bar_renderer.Draw(canvas, rc, Basic(), Calculated(),
                                GetComputerSettings().polar.glide_polar_task,
                                true); //NOTE: AVG enabled for now, make it configurable ;
}

void
GlueMapWindow::DrawMapScale(Canvas &canvas, const PixelRect &rc,
                            const MapWindowProjection &projection) const
{
  RenderMapScale(canvas, projection, rc, look.overlay);

  if (!projection.IsValid())
    return;

  StaticString<80> buffer;

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

  if (rasp_renderer != nullptr) {
    const TCHAR *label = rasp_renderer->GetLabel();
    if (label != nullptr)
      buffer += gettext(label);
  }

  if (!buffer.empty()) {

    const Font &font = *look.overlay.overlay_font;
    canvas.Select(font);
    const unsigned height = font.GetCapitalHeight()
        + Layout::GetTextPadding();
    int y = rc.bottom - height;

    TextInBoxMode mode;
    mode.vertical_position = TextInBoxMode::VerticalPosition::ABOVE;
    mode.shape = LabelShape::OUTLINED;

    TextInBox(canvas, buffer, 0, y, mode, rc, nullptr);
  }
}

void
GlueMapWindow::DrawThermalEstimate(Canvas &canvas) const
{
  if (InCirclingMode() && IsNearSelf()) {
    // in circling mode, draw thermal at actual estimated location
    const MapWindowProjection &projection = render_projection;
    const ThermalLocatorInfo &thermal_locator = Calculated().thermal_locator;
    if (thermal_locator.estimate_valid) {
      PixelPoint sc;
      if (projection.GeoToScreenIfVisible(thermal_locator.estimate_location, sc)) {
        look.thermal_source_icon.Draw(canvas, sc);
      }
    }
  } else {
    MapWindow::DrawThermalEstimate(canvas);
  }
}

void
GlueMapWindow::RenderTrail(Canvas &canvas, const PixelPoint aircraft_pos)
{
  unsigned min_time;
  switch(GetMapSettings().trail.length) {
  case TrailSettings::Length::OFF:
    return;
  case TrailSettings::Length::LONG:
    min_time = std::max(0, (int)Basic().time - 3600);
    break;
  case TrailSettings::Length::SHORT:
    min_time = std::max(0, (int)Basic().time - 600);
    break;
  case TrailSettings::Length::FULL:
  default:
    min_time = 0; // full
    break;
  }

  DrawTrail(canvas, aircraft_pos, min_time,
            GetMapSettings().trail.wind_drift_enabled && InCirclingMode());
}

void
GlueMapWindow::RenderTrackBearing(Canvas &canvas, const PixelPoint aircraft_pos)
{
  DrawTrackBearing(canvas, aircraft_pos, InCirclingMode());
}

void
GlueMapWindow::DrawThermalBand(Canvas &canvas, const PixelRect &rc) const
{
  if (Calculated().task_stats.total.solution_remaining.IsOk() &&
      Calculated().task_stats.total.solution_remaining.altitude_difference > 50
      && GetDisplayMode() == DisplayMode::FINAL_GLIDE)
    return;

  PixelRect tb_rect;
  tb_rect.left = rc.left;
  tb_rect.right = rc.left+Layout::Scale(25);
  tb_rect.top = Layout::Scale(2);
  tb_rect.bottom = (rc.bottom-rc.top)/5 - Layout::Scale(2);

  const ThermalBandRenderer &renderer = thermal_band_renderer;
  if (task != nullptr) {
    ProtectedTaskManager::Lease task_manager(*task);
    renderer.DrawThermalBand(Basic(),
                             Calculated(),
                             GetComputerSettings(),
                             canvas,
                             tb_rect,
                             GetComputerSettings().task,
                             true,
                             &task_manager->GetOrderedTask().GetOrderedTaskSettings());
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
    auto s = Clamp(Basic().stall_ratio, 0., 1.);
    int m = rc.GetHeight() * s * s;

    canvas.SelectBlackPen();
    canvas.DrawLine(rc.right - 1, rc.bottom - m, rc.right - 11, rc.bottom - m);
  }
}
