// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlueMapWindow.hpp"
#include "Look/MapLook.hpp"
#include "ui/canvas/Icon.hpp"
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
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "Look/GestureLook.hpp"
#include "Input/InputEvents.hpp"
#include "Renderer/MapScaleRenderer.hpp"
#include "net/State.hpp"

#include <algorithm> // for std::clamp()

void
GlueMapWindow::DrawGesture(Canvas &canvas) const noexcept
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
GlueMapWindow::DrawCrossHairs(Canvas &canvas) const noexcept
{
  if (!render_projection.IsValid())
    return;

  canvas.Select(look.overlay.crosshair_pen);

  const auto center = render_projection.GetScreenOrigin();

  auto FullLength = Layout::FastScale(20);
  auto HalfLength = Layout::FastScale(10);
  auto EdgeLength = Layout::FastScale(30);

  // Edges
  canvas.DrawLine(center.At(FullLength, FullLength), center.At(HalfLength, FullLength));
  canvas.DrawLine(center.At(FullLength, FullLength), center.At(FullLength, HalfLength));

  canvas.DrawLine(center.At(-FullLength, FullLength), center.At(-HalfLength, FullLength));
  canvas.DrawLine(center.At(-FullLength, FullLength), center.At(-FullLength, HalfLength));

  canvas.DrawLine(center.At(-FullLength, -FullLength), center.At(-HalfLength, -FullLength));
  canvas.DrawLine(center.At(-FullLength, -FullLength), center.At(-FullLength, -HalfLength));

  canvas.DrawLine(center.At(FullLength, -FullLength), center.At(FullLength, -HalfLength));
  canvas.DrawLine(center.At(FullLength, -FullLength), center.At(HalfLength, -FullLength));

  // Crosshair
  canvas.Select(look.overlay.crosshair_pen_alias);
  canvas.DrawLine(center.At(0, -EdgeLength), center.At(0, -HalfLength));
  canvas.DrawLine(center.At(0, EdgeLength), center.At(0, HalfLength));

  canvas.DrawLine(center.At(-EdgeLength, 0), center.At(-HalfLength, 0));
  canvas.DrawLine(center.At(EdgeLength, 0), center.At(HalfLength, 0));

}

void
GlueMapWindow::DrawPanInfo(Canvas &canvas) const noexcept
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
  PixelPoint p(render_projection.GetScreenSize().width - padding, padding);

  if (compass_visible)
    /* don't obscure the north arrow */
    /* TODO: obtain offset from CompassRenderer */
    p.y += Layout::Scale(19) + Layout::FastScale(13);

  if (terrain) {
    TerrainHeight elevation = terrain->GetTerrainHeight(location);
    if (!elevation.IsSpecial()) {
      StaticString<64> elevation_long;
      elevation_long = _("Elevation: ");
      elevation_long += FormatUserAltitude(elevation.GetValue()).c_str();

      TextInBox(canvas, elevation_long, p, mode,
                render_projection.GetScreenSize());

      p.y += height;
    }
  }

  TCHAR buffer[256];
  FormatGeoPoint(location, buffer, ARRAY_SIZE(buffer), _T('\n'));

  TCHAR *start = buffer;
  while (true) {
    auto *newline = StringFind(start, _T('\n'));
    if (newline != nullptr)
      *newline = _T('\0');

    TextInBox(canvas, start, p, mode, render_projection.GetScreenSize());

    p.y += height;

    if (newline == nullptr)
      break;

    start = newline + 1;
  }
}

void
GlueMapWindow::DrawSystemStatus(Canvas &canvas, const PixelRect &rc) const noexcept
{
  PixelPoint p(rc.left + Layout::FastScale(4), rc.top + Layout::FastScale(4));
  int offset_x = 0;

  const NetState net_state = GetNetState();
  const MaskedIcon *net_icon = nullptr;
  if (net_state == NetState::CONNECTED || net_state == NetState::ROAMING)
    net_icon = &look.network_connected_icon;
  else if (net_state == NetState::DISCONNECTED)
    net_icon = &look.network_disconnected_icon;
  else
    return; // no icon for UNKNOWN state

  net_icon->Draw(canvas, p);
  offset_x += net_icon->GetSize().width + Layout::FastScale(4);

  const auto &info = Basic();
  const MaskedIcon *gps_icon = nullptr;
  const TCHAR *gps_txt = nullptr;

  if (!info.alive) {
    gps_icon = &look.no_gps_icon;
    gps_txt = _("GPS not connected");
  } else if (!info.location_available) {
    gps_icon = &look.waiting_for_fix_icon;
    gps_txt = _("GPS waiting for fix");
  }

  if (gps_icon != nullptr) {
    PixelPoint gps_p = p;
    gps_p.x += offset_x;
    gps_icon->Draw(canvas, gps_p);

    if (gps_txt != nullptr) {
      const Font &font = *look.overlay.overlay_font;
      canvas.Select(font);

      PixelPoint text_p = gps_p;
      text_p.x += gps_icon->GetSize().width + Layout::FastScale(4);
      const int text_height = font.GetHeight();
      const int icon_height = gps_icon->GetSize().height;
      text_p.y += std::max(0, (icon_height - text_height) / 2);

      TextInBoxMode mode;
      mode.shape = LabelShape::ROUNDED_BLACK;

      TextInBox(canvas, gps_txt, text_p, mode, rc, nullptr);
    }
  }
}

void
GlueMapWindow::DrawFlightMode(Canvas &canvas,
                              const PixelRect &rc) const noexcept
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

  offset += bmp->GetSize().width + Layout::Scale(6);

  bmp->Draw(canvas,
            PixelPoint(rc.right - offset,
                       rc.bottom - bmp->GetSize().height - Layout::Scale(4)));

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

  offset += bmp->GetSize().width + Layout::Scale(6);

  bmp->Draw(canvas,
            PixelPoint(rc.right - offset,
                       rc.bottom - bmp->GetSize().height - Layout::Scale(2)));
}

void
GlueMapWindow::DrawFinalGlide(Canvas &canvas,
                              const PixelRect &rc) const noexcept
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
GlueMapWindow::DrawVario(Canvas &canvas, const PixelRect &rc) const noexcept
{
  if (!GetMapSettings().vario_bar_enabled)
   return;

  vario_bar_renderer.Draw(canvas, rc, Basic(), Calculated(),
                                GetComputerSettings().polar.glide_polar_task,
                                true); //NOTE: AVG enabled for now, make it configurable ;
}

void
GlueMapWindow::SetBottomMargin(unsigned margin) noexcept
{
  if (margin == bottom_margin)
    /* no change, don't redraw */
    return;

  bottom_margin = margin;
  QuickRedraw();
}

void
GlueMapWindow::SetBottomMarginFactor(unsigned margin_factor) noexcept
{
  if (margin_factor == 0) {
    SetBottomMargin(0);
    return;
  }

  PixelRect map_rect = GetClientRect();

  if (map_rect.GetHeight() > map_rect.GetWidth()) {
    SetBottomMargin(map_rect.bottom / margin_factor);
  } else {
    SetBottomMargin(0);
  }
}

void
GlueMapWindow::DrawMapScale(Canvas &canvas, const PixelRect &rc,
                            const MapWindowProjection &projection) const noexcept
{

  PixelRect scale_pos(rc.left, rc.top, rc.right, rc.bottom - bottom_margin);

  RenderMapScale(canvas, projection, scale_pos, look.overlay);

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
    const int height = font.GetCapitalHeight()
        + Layout::GetTextPadding();

    TextInBoxMode mode;
    mode.vertical_position = TextInBoxMode::VerticalPosition::ABOVE;
    mode.shape = LabelShape::OUTLINED;

    TextInBox(canvas, buffer, {0, scale_pos.bottom - height}, mode, rc, nullptr);
  }
}

void
GlueMapWindow::DrawThermalEstimate(Canvas &canvas) const noexcept
{
  if (InCirclingMode() && IsNearSelf()) {
    // in circling mode, draw thermal at actual estimated location
    const MapWindowProjection &projection = render_projection;
    const ThermalLocatorInfo &thermal_locator = Calculated().thermal_locator;
    if (thermal_locator.estimate_valid) {
      if (auto p = projection.GeoToScreenIfVisible(thermal_locator.estimate_location)) {
        look.thermal_source_icon.Draw(canvas, *p);
      }
    }
  } else {
    MapWindow::DrawThermalEstimate(canvas);
  }
}

void
GlueMapWindow::RenderTrail(Canvas &canvas,
                           const PixelPoint aircraft_pos) noexcept
{
  TimeStamp min_time;
  switch(GetMapSettings().trail.length) {
  case TrailSettings::Length::OFF:
    return;
  case TrailSettings::Length::LONG:
    min_time = std::max(Basic().time - std::chrono::hours{1}, TimeStamp{});
    break;
  case TrailSettings::Length::SHORT:
    min_time = std::max(Basic().time - std::chrono::minutes{10}, TimeStamp{});
    break;
  case TrailSettings::Length::FULL:
  default:
    min_time = {}; // full
    break;
  }

  DrawTrail(canvas, aircraft_pos, min_time,
            GetMapSettings().trail.wind_drift_enabled && InCirclingMode());
}

void
GlueMapWindow::RenderTrackBearing(Canvas &canvas,
                                  const PixelPoint aircraft_pos) noexcept
{
  DrawTrackBearing(canvas, aircraft_pos, InCirclingMode());
}

void
GlueMapWindow::DrawThermalBand(Canvas &canvas,
                               const PixelRect &rc) const noexcept
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
GlueMapWindow::DrawStallRatio(Canvas &canvas,
                              const PixelRect &rc) const noexcept
{
  if (Basic().stall_ratio_available) {
    // JMW experimental, display stall sensor
    auto s = std::clamp(Basic().stall_ratio, 0., 1.);
    int m = rc.GetHeight() * s * s;

    const auto p = rc.GetBottomRight();

    canvas.SelectBlackPen();
    canvas.DrawLine(p.At(-1, -m), p.At(-11, -m));
  }
}
