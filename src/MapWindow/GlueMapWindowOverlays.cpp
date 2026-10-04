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
#include "Renderer/GestureRenderer.hpp"
#include "Input/InputEvents.hpp"
#include "Renderer/MapScaleRenderer.hpp"
#include "Renderer/CompassRenderer.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Replay/Replay.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "Asset.hpp"
#include "Look/Colors.hpp"

#ifdef ENABLE_OPENGL
#include "Hardware/CPU.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#ifdef DEBUG_PROCESS_LOAD
#include "Topography/TopographyFileRenderer.hpp"
#endif
#endif

#ifdef DEBUG_PROCESS_LOAD
#include <array>
#include <chrono>
#include <string_view>
#include <fmt/format.h>
#ifdef _WIN32
#include <processthreadsapi.h>
#else
#include <time.h>
#endif
#endif

#include <algorithm> // for std::clamp()

#if DEBUG_ALL_MAP_OVERLAYS
#include "Engine/Task/Stats/ElementStat.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "time/Stamp.hpp"

#include <cmath>

/*
 * Feeds the bar renderers synthetic data so every overlay is on screen
 * at once and each bar reaches its full extent in both directions.
 */
[[gnu::pure]]
static DerivedInfo
DebugFinalGlideData(DerivedInfo calculated) noexcept
{
  /* The renderer bails out without a valid task.  ±468 m drives the
     two bars to opposite ends of the range, so both clipping arrows
     show; mc0 must be valid, or only the upward bar is drawn. */
  ElementStat &total = calculated.task_stats.total;

  calculated.task_stats.task_valid = true;
  total.solution_remaining.validity = GlideResult::Validity::OK;
  total.solution_remaining.altitude_difference = 468;
  total.solution_remaining.pure_glide_altitude_difference = 468;
  total.solution_mc0.validity = GlideResult::Validity::OK;
  total.solution_mc0.altitude_difference = -468;
  total.solution_mc0.pure_glide_altitude_difference = -468;

  return calculated;
}

/**
 * A wavy climb from 500 m to about 2 km so the thermal profile has
 * visible shape without a real flight.
 */
[[gnu::pure]]
static DerivedInfo
DebugThermalBandData(DerivedInfo calculated) noexcept
{
  auto &band = calculated.thermal_encounter_band;
  band.Reset();

  TimeStamp t{FloatDuration{100}};
  double h = 500;
  band.AddSample(t, h);

  for (unsigned i = 1; i <= 36; ++i) {
    const double w = 1.5 + 1.8 * std::sin(i * 0.35)
      + 0.6 * std::sin(i * 0.11);
    const double dh = 45;
    t += FloatDuration{dh / std::max(0.4, w)};
    h += dh;
    band.AddSample(t, h);
  }

  calculated.common_stats.height_min_working = 500;
  calculated.common_stats.height_max_working = h;
  return calculated;
}

[[gnu::pure]]
static MoreData
DebugThermalBandBasic(MoreData basic) noexcept
{
  /* mid-band altitude so the MC tick sits in the profile */
  basic.gps_altitude = 1200;
  basic.gps_altitude_available.Update(basic.clock);
  basic.nav_altitude = 1200;
  return basic;
}
#endif

void
GlueMapWindow::DrawGesture(Canvas &canvas) const noexcept
{
  const char *gesture = nullptr;
  if (gestures.HasPoints()) {
    gesture = gestures.GetGesture();
    const bool valid = gesture == nullptr || InputEvents::IsGesture(gesture);

    GestureRenderer::Draw(canvas, gesture_look, gestures.GetPoints(), valid);
  } else if (!DEBUG_ALL_MAP_OVERLAYS)
    return;

  /* name the action which lifting the finger now would trigger */
  const char *label = gesture != nullptr
    ? InputEvents::GetGestureLabel(gesture)
    : nullptr;
  if (label == nullptr) {
    if (!DEBUG_ALL_MAP_OVERLAYS)
      return;
    label = "Gesture";
  }

  canvas.Select(*look.overlay.overlay_font);

  /* the same area as the compass and the map scale, so the label
     stays clear of the system bars, the cutout and the InfoBoxes
     when the map reaches past them */
  const PixelRect rc = GetHudRect();

  TextInBoxMode mode;
  mode.shape = LabelShape::PILL;
  mode.align = TextInBoxMode::Alignment::CENTER;
  mode.move_in_view = true;

  TextInBox(canvas, label, {rc.GetCenter().x, rc.top + Layout::Scale(12)},
            mode, rc);
}

/**
 * How long the page indicator is visible after a page switch,
 * including #PAGE_INDICATOR_FADE_DURATION.
 */
static constexpr std::chrono::milliseconds PAGE_INDICATOR_DURATION{2000};

/**
 * How long the page indicator takes to fade out.
 */
static constexpr std::chrono::milliseconds PAGE_INDICATOR_FADE_DURATION{300};

/**
 * Does the page indicator fade out, or does it just disappear?  Each
 * step of the fade repaints the map window, which a slow CPU would
 * only stutter through, and e-paper would only show ghosting.
 */
[[gnu::pure]]
static bool
PageIndicatorFades() noexcept
{
#ifdef ENABLE_OPENGL
  return !HasEPaper() && !IsSlowCPU();
#else
  return false;
#endif
}

void
GlueMapWindow::DrawPageIndicator(Canvas &canvas) const noexcept
{
  unsigned n_pages = page_indicator_count;
  unsigned current = page_indicator_index;
  if (DEBUG_ALL_MAP_OVERLAYS) {
    /* stay visible, with a sample row when this page has no neighbours */
    if (n_pages < 2 || current >= n_pages) {
      n_pages = 4;
      current = 1;
    }
  } else if (n_pages < 2 || current >= n_pages)
    return;

  uint8_t opacity = 0xff;
  if (!DEBUG_ALL_MAP_OVERLAYS) {
    const auto remaining = PAGE_INDICATOR_DURATION -
      (std::chrono::steady_clock::now() - page_indicator_time);
    if (remaining <= remaining.zero())
      return;

    if (PageIndicatorFades() && remaining < PAGE_INDICATOR_FADE_DURATION)
      opacity = uint8_t(0xff * (std::chrono::duration<double>(remaining) /
                                PAGE_INDICATOR_FADE_DURATION));
  }

  const Color color = ColorWithAlpha(COLOR_BLACK, opacity);

  /* dots of about 2.5 mm, 1.8 mm apart */
  const unsigned dot = Layout::VptScale(7);
  const unsigned gap = Layout::VptScale(5);
  const unsigned height = dot + 2 * Layout::VptScale(5);

  /* the round ends of the pill are half its height wide */
  const unsigned width = n_pages * dot + (n_pages - 1) * gap + height;

  /* bottom centre of the HUD, as far from its bottom as the gesture
     label is from its top (see DrawGesture()); the map scale is on
     the left, the flight mode icon on the right */
  const PixelRect rc = GetHudRect();
  PixelRect pill;
  pill.left = rc.GetCenter().x - int(width / 2);
  pill.right = pill.left + int(width);
  pill.bottom = rc.bottom - int(bottom_margin) -
    (Layout::Scale(12) - int(Layout::GetTextPadding()));
  pill.top = pill.bottom - int(height);

  DrawPill(canvas, pill, opacity);

#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend alpha_blend;
#endif

  /* the current page is a filled dot, the others are rings: the shape
     tells them apart, not the colour, which works on e-paper, too */
  const unsigned pen_width = Layout::ScaleFinePenWidth(1);
  canvas.Select(Pen(pen_width, color));
  const Brush brush(color);

  const unsigned radius = (dot - pen_width) / 2;
  PixelPoint center{pill.left + int(height / 2 + dot / 2),
                    pill.GetCenter().y};

  for (unsigned i = 0; i < n_pages; ++i) {
    if (i == current)
      canvas.Select(brush);
    else
      canvas.SelectHollowBrush();

    canvas.DrawCircle(center, radius);
    center.x += int(dot + gap);
  }
}

void
GlueMapWindow::OnPageIndicatorTimer() noexcept
{
  const auto remaining = PAGE_INDICATOR_DURATION -
    (std::chrono::steady_clock::now() - page_indicator_time);

  if (remaining <= remaining.zero()) {
    /* remove the page indicator */
    PaintWindow::Invalidate();
    return;
  }

  using Duration = std::chrono::steady_clock::duration;
  const Duration fade = PageIndicatorFades()
    ? Duration{PAGE_INDICATOR_FADE_DURATION}
    : Duration::zero();

  if (remaining > fade) {
    /* keep it until it starts fading out (or disappears) */
    page_indicator_timer.Schedule(remaining - fade);
    return;
  }

  /* one step of the fade */
  PaintWindow::Invalidate();
  page_indicator_timer.Schedule(std::min<Duration>(
    remaining, std::chrono::milliseconds{40}));
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
GlueMapWindow::DrawPanInfo(Canvas &canvas,
                           const MapHudLayout &layout) const noexcept
{
  if (!render_projection.IsValid())
    return;

  GeoPoint location = render_projection.GetGeoLocation();

  TextInBoxMode mode;
  mode.shape = LabelShape::OUTLINED;
  mode.align = TextInBoxMode::Alignment::RIGHT;

  const Font &font = *look.overlay.overlay_font;
  canvas.Select(font);

  unsigned height = font.GetHeight();
  const PixelRect &rc = layout.top_right;
  PixelPoint p = layout.GetPanInfoOrigin(compass_visible);

  if (terrain) {
    TerrainHeight elevation = terrain->GetTerrainHeight(location);
    if (!elevation.IsSpecial()) {
      StaticString<64> elevation_long;
      elevation_long.Format("%s: %s", _("Elevation"),
                            FormatUserAltitude(elevation.GetValue()).c_str());

      TextInBox(canvas, elevation_long, p, mode, rc);

      p.y += height;
    }
  }

  char buffer[256];
  FormatGeoPoint(location, buffer, ARRAY_SIZE(buffer), '\n');

  char *start = buffer;
  while (true) {
    auto *newline = StringFind(start, '\n');
    if (newline != nullptr)
      *newline = '\0';

    TextInBox(canvas, start, p, mode, rc);

    p.y += height;

    if (newline == nullptr)
      break;

    start = newline + 1;
  }

  /* RASP field value at the panned location, analogous to the "map
     items at this location" dialog. */
  if (rasp_renderer && rasp_renderer->IsInside(location)) {
    const char *label = rasp_renderer->GetLabel();
    if (label != nullptr && *label != '\0') {
      const auto value = FormatRaspValue(rasp_renderer->GetValueAt(location));

      StaticString<128> rasp_line;
      if (value.empty())
        rasp_line = label;
      else
        rasp_line.Format("%s: %s", label, value.c_str());

      TextInBox(canvas, rasp_line, p, mode, rc);

      p.y += height;
    }
  }
}

void
GlueMapWindow::DrawGPSStatus(Canvas &canvas, const MapHudLayout &layout,
                             const NMEAInfo &info) const noexcept
{
  const char *txt;
  const MaskedIcon *icon;

  if (!info.alive) {
    icon = &look.no_gps_icon;
    txt = _("GPS not connected");
  } else if (!info.location_available) {
    icon = &look.waiting_for_fix_icon;
    txt = _("GPS waiting for fix");
  } else if (DEBUG_ALL_MAP_OVERLAYS) {
    icon = &look.waiting_for_fix_icon;
    txt = "GPS status";
  } else
    // early exit
    return;

  const Font &font = *look.overlay.overlay_font;
  canvas.Select(font);

  const PixelRect &area = layout.bottom;
  const int clear_bottom = area.bottom - int(layout.scale_title_clearance);

  const int row_height = std::max((int)icon->GetSize().height,
                                  (int)font.GetHeight());
  PixelPoint p(area.left, clear_bottom - row_height);
  icon->Draw(canvas, p);

  p.x += icon->GetSize().width + Layout::FastScale(4);
  p.y = clear_bottom - (int)font.GetAscentHeight()
    - ((row_height - (int)font.GetHeight()) / 2);

  TextInBoxMode mode;
  mode.shape = LabelShape::ROUNDED_BLACK;

  TextInBox(canvas, txt, p, mode, area, nullptr);
}

void
GlueMapWindow::DrawFlightMode(Canvas &canvas,
                              const MapHudLayout &layout) const noexcept
{
  const PixelRect &area = layout.bottom;

  int offset = 0;
  const int gap = int(Layout::GetTextPadding());

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

  offset += int(bmp->GetSize().width);

  bmp->Draw(canvas,
            PixelPoint(area.right - offset,
                       area.bottom - int(bmp->GetSize().height)));

  // draw flarm status
  if (!GetMapSettings().show_flarm_alarm_level && !DEBUG_ALL_MAP_OVERLAYS)
    // Don't show indicator when the gauge is indicating the traffic anyway
    return;

  const FlarmStatus &flarm = Basic().flarm.status;
  if (!flarm.available) {
    if (!DEBUG_ALL_MAP_OVERLAYS)
      return;

    bmp = &look.traffic_safe_icon;
  } else
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

  offset += int(bmp->GetSize().width) + gap;

  bmp->Draw(canvas,
            PixelPoint(area.right - offset,
                       area.bottom - int(bmp->GetSize().height)));
}

void
GlueMapWindow::DrawFinalGlide(Canvas &canvas,
                              const MapHudLayout &layout) const noexcept
{
  const GlideSettings &glide_settings = GetComputerSettings().task.glide;
  const PixelRect &area = layout.bottom;

#if DEBUG_ALL_MAP_OVERLAYS
  final_glide_bar_renderer.Draw(canvas, area,
                                DebugFinalGlideData(Calculated()),
                                glide_settings, true);
  return;
#else

  if (GetMapSettings().final_glide_bar_display_mode==FinalGlideBarDisplayMode::OFF)
    return;

  if (GetMapSettings().final_glide_bar_display_mode==FinalGlideBarDisplayMode::AUTO) {
    const TaskStats &task_stats = Calculated().task_stats;
    const ElementStat &total = task_stats.total;
    const GlideResult &solution = total.solution_remaining;
    const GlideResult &solution_mc0 = total.solution_mc0;

    if (!task_stats.task_valid || !solution.IsOk() || !solution_mc0.IsDefined())
      return;

    if (solution_mc0.SelectAltitudeDifference(glide_settings) < -1000 &&
        solution.SelectAltitudeDifference(glide_settings) < -1000)
      return;
  }

  final_glide_bar_renderer.Draw(canvas, area, Calculated(),
                                glide_settings,
                                GetMapSettings().final_glide_bar_mc0_enabled);
#endif
}

void
GlueMapWindow::DrawVario(Canvas &canvas,
                         const MapHudLayout &layout) const noexcept
{
  const GlidePolar &polar = GetComputerSettings().polar.glide_polar_task;
  const PixelRect &area = layout.bottom;

#if DEBUG_ALL_MAP_OVERLAYS
  /* gross and average vario at opposite ends of the ±5 m/s range, so
     both bars reach their full extent in both directions */
  MoreData basic = Basic();
  DerivedInfo calculated = Calculated();
  basic.brutto_vario = basic.filtered_brutto_vario = 5;
  calculated.average = -5;

  vario_bar_renderer.Draw(canvas, area, basic, calculated, polar, true);
#else
  if (!GetMapSettings().vario_bar_enabled)
   return;

  vario_bar_renderer.Draw(canvas, area, Basic(), Calculated(),
                                polar,
                                true); //NOTE: AVG enabled for now, make it configurable ;
#endif
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
GlueMapWindow::SetTopRightMargin(unsigned margin) noexcept
{
  if (margin == top_right_margin)
    /* no change, don't redraw */
    return;

  top_right_margin = margin;
  QuickRedraw();
}

MapHudLayout
GlueMapWindow::GetHudLayout(PixelRect hud_rc) const noexcept
{
  return MapHudLayout::Build(hud_rc,
                             top_right_margin,
                             bottom_margin,
                             CompassRenderer::GetSlotHeight(),
                             GetMapScaleAndTitleClearance(*look.overlay.overlay_font));
}

void
GlueMapWindow::SetBottomMarginFactor(unsigned margin_factor) noexcept
{
  if (follow_mode != FOLLOW_PAN || Layout::landscape) {
    /* only apply bottom margin in portrait pan mode where
       overlay buttons cover the bottom of the screen */
    SetBottomMargin(0);
    return;
  }

  if (margin_factor == 0) {
    SetBottomMargin(0);
    return;
  }

  PixelRect parent_rect = GetParentClientRect();
  unsigned screen_height = parent_rect.GetHeight();

  SetBottomMargin(screen_height / margin_factor);
}

void
GlueMapWindow::DrawMapScale(Canvas &canvas, const MapHudLayout &layout,
                            const MapWindowProjection &projection) const noexcept
{
  const PixelRect &scale_pos = layout.bottom;

  unsigned contour_spacing_m = 0;
  const auto &terrain = GetMapSettings().terrain;
  if (projection.IsValid() &&
      terrain.enable && terrain.contours != Contours::OFF &&
      background.AreContoursVisible())
    contour_spacing_m = background.GetContourSpacing();

  RenderMapScale(canvas, projection, scale_pos, look.overlay, contour_spacing_m);

  if (!projection.IsValid())
    return;

  StaticString<256> buffer;

  buffer.clear();

  if (GetMapSettings().auto_zoom_enabled)
    buffer = "AUTO ";

  switch (follow_mode) {
  case FOLLOW_SELF:
    break;

  case FOLLOW_PAN:
    buffer += "PAN ";
    break;
  }

  const UIState &ui_state = GetUIState();
  if (Basic().gps.replay) {
    if (backend_components != nullptr &&
        backend_components->replay != nullptr)
      buffer.AppendFormat(_("REPLAY %.0fx "),
                          backend_components->replay->GetTimeScale());
    else
      buffer += _("REPLAY ");
  } else if (Basic().gps.simulator) {
    buffer += _("Simulator");
    buffer += " ";
  }

  if (!ui_state.map_scale_page_title.empty()) {
    buffer += "| ";
    buffer += ui_state.map_scale_page_title;
    buffer += " ";
  } else if (ui_state.auxiliary_enabled) {
    buffer += ui_state.panel_name;
    buffer += " ";
  }

  if (GetComputerSettings().polar.ballast_timer_active)
    buffer.AppendFormat(
        "BALLAST %d LITERS ",
        (int)GetComputerSettings().polar.glide_polar_task.GetBallastLitres());

  if (buffer.empty() && DEBUG_ALL_MAP_OVERLAYS)
    buffer = "Map title";

  if (!buffer.empty()) {

    const Font &font = *look.overlay.overlay_font;
    canvas.Select(font);
    const int height = int(GetMapScaleBandHeight(font));

    TextInBoxMode mode;
    mode.vertical_position = TextInBoxMode::VerticalPosition::ABOVE;
    mode.shape = LabelShape::OUTLINED;

    /* the same left edge as the scale bar, which is the HUD, not the
       map window: the map runs under the system bars and the InfoBoxes */
    TextInBox(canvas, buffer,
              {scale_pos.left, scale_pos.bottom - height},
              mode, scale_pos, nullptr);
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

  /* Trail drift is for thermal centering at near zoom.  At overview
     scales a Full trail shifted by hours of wind looks like a wrong
     ground track (#2835, #709).  GetMapScale 2000 ≈ 16 km short edge. */
  static constexpr double TRAIL_DRIFT_MAX_MAP_SCALE = 2000;

  const bool enable_traildrift =
    GetMapSettings().trail.wind_drift_enabled &&
    InCirclingMode() &&
    render_projection.GetMapScale() <= TRAIL_DRIFT_MAX_MAP_SCALE;

  DrawTrail(canvas, aircraft_pos, min_time, enable_traildrift);
}

void
GlueMapWindow::RenderTrackBearing(Canvas &canvas,
                                  const PixelPoint aircraft_pos) noexcept
{
  DrawTrackBearing(canvas, aircraft_pos, InCirclingMode());
}

void
GlueMapWindow::DrawThermalBand(Canvas &canvas,
                               const MapHudLayout &layout) const noexcept
{
  if (!DEBUG_ALL_MAP_OVERLAYS &&
      Calculated().task_stats.total.solution_remaining.IsOk() &&
      Calculated().task_stats.total.solution_remaining.altitude_difference > 50
      && GetDisplayMode() == DisplayMode::FINAL_GLIDE)
    return;

  const PixelRect tb_rect = layout.GetThermalBandRect();

  const ThermalBandRenderer &renderer = thermal_band_renderer;

#if DEBUG_ALL_MAP_OVERLAYS
  renderer.DrawThermalBand(DebugThermalBandBasic(Basic()),
                           DebugThermalBandData(Calculated()),
                           GetComputerSettings(),
                           canvas,
                           tb_rect,
                           GetComputerSettings().task,
                           true);
  return;
#else

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
#endif
}

void
GlueMapWindow::DrawStallRatio(Canvas &canvas,
                              const MapHudLayout &layout) const noexcept
{
  // JMW experimental, display stall sensor
  if (!Basic().stall_ratio_available && !DEBUG_ALL_MAP_OVERLAYS)
    return;

  const auto s = DEBUG_ALL_MAP_OVERLAYS
    ? 0.5
    : std::clamp(Basic().stall_ratio, 0., 1.);
  const PixelRect &area = layout.bottom;
  const int m = area.GetHeight() * s * s;

  const auto p = area.GetBottomRight();

  canvas.SelectBlackPen();
  canvas.DrawLine(p.At(-1, -m), p.At(-11, -m));
}

#ifdef DEBUG_PROCESS_LOAD

static constexpr unsigned LOAD_HISTORY = 64;

struct ProcessLoadHistory {
  std::array<unsigned, LOAD_HISTORY> cpu_pct{};
  std::array<unsigned, LOAD_HISTORY> gpu_ms{};
  unsigned i = 0;
  unsigned n = 0;
};

static ProcessLoadHistory load_history;

[[nodiscard]]
static unsigned
SampleProcessCpuPercent() noexcept
{
  static bool have = false;
#ifdef _WIN32
  static uint64_t last_proc = 0;
  static unsigned last_tick = 0;

  uint64_t creation, exit_t, kernel, user;
  if (!GetProcessTimes(GetCurrentProcess(),
                       (FILETIME *)&creation, (FILETIME *)&exit_t,
                       (FILETIME *)&kernel, (FILETIME *)&user))
    return 0;

  const uint64_t proc = (kernel + user) / 10000;
  const unsigned tick = GetTickCount();
  unsigned cpu = 0;
  if (have && tick > last_tick)
    cpu = unsigned((100 * (proc - last_proc)) / (tick - last_tick));
  last_proc = proc;
  last_tick = tick;
  have = true;
  return cpu;
#else
  static std::chrono::steady_clock::time_point last_wall;
  static std::chrono::nanoseconds last_proc{0};

  timespec ts{};
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0)
    return 0;

  const auto now = std::chrono::steady_clock::now();
  const auto proc = std::chrono::seconds(ts.tv_sec) +
    std::chrono::nanoseconds(ts.tv_nsec);
  unsigned cpu = 0;
  if (have) {
    const auto dw = now - last_wall;
    const auto dp = proc - last_proc;
    if (dw.count() > 0)
      cpu = unsigned((100.0 * double(dp.count()) /
                      double(dw.count())) + 0.5);
  }
  last_wall = now;
  last_proc = proc;
  have = true;
  return cpu;
#endif
}

static void
DrawSparkline(Canvas &canvas, PixelRect rc,
              const unsigned *hist, unsigned n, unsigned pos,
              unsigned ymax, const Pen &pen) noexcept
{
  if (n < 2 || ymax == 0 || rc.GetWidth() < 2 || rc.GetHeight() < 2)
    return;

  BulkPixelPoint pts[LOAD_HISTORY];
  const unsigned h = rc.GetHeight();
  const unsigned w = rc.GetWidth();
  for (unsigned s = 0; s < n; ++s) {
    const unsigned idx = (pos + LOAD_HISTORY - n + s) % LOAD_HISTORY;
    const unsigned v = std::min(hist[idx], ymax);
    pts[s].x = rc.left + int(s * (w - 1) / (n - 1));
    pts[s].y = rc.bottom - 1 - int(v * (h - 1) / ymax);
  }

  canvas.Select(pen);
  canvas.DrawPolyline(pts, n);
}

void
GlueMapWindow::DrawProcessLoad(Canvas &canvas, const MapHudLayout &layout,
                               unsigned frame_ms) noexcept
{
  if (HasEPaper())
    return;

  /* One sample per paint is noisy: MergeThread and CalculationThread
     bursts look like idle spikes.  Average over ~250 ms instead. */
  static unsigned cpu = 0;
  static unsigned gpu_max_ms = 0;
  static std::chrono::steady_clock::time_point last_sample;
  static bool have_sample = false;

  gpu_max_ms = std::max(gpu_max_ms, std::min(frame_ms, 100u));

  const auto now = std::chrono::steady_clock::now();
  if (!have_sample ||
      now - last_sample >= std::chrono::milliseconds(250)) {
    cpu = SampleProcessCpuPercent();
    load_history.cpu_pct[load_history.i] = cpu;
    load_history.gpu_ms[load_history.i] = gpu_max_ms;
    load_history.i = (load_history.i + 1) % LOAD_HISTORY;
    if (load_history.n < LOAD_HISTORY)
      ++load_history.n;
    gpu_max_ms = 0;
    last_sample = now;
    have_sample = true;
  }

  const unsigned graph_w = Layout::Scale(140);
  const unsigned graph_h = Layout::Scale(28);
  const unsigned text_h = look.overlay.overlay_font->GetHeight();
#ifdef ENABLE_OPENGL
  const unsigned n_layers = GetLastTopographyLayerVtxCount();
  unsigned n_show = 0;
  const auto *layers = GetLastTopographyLayerVtx();
  for (unsigned i = 0; i < n_layers && n_show < 4; ++i)
    if (layers[i].vertices > 0)
      ++n_show;
#else
  const unsigned n_show = 0;
#endif
  const unsigned box_h = graph_h * 2 + text_h * (2 + n_show) +
    Layout::Scale(8);

  PixelRect box = layout.top_right;
  if (int(box.GetWidth()) < int(graph_w) ||
      int(box.GetHeight()) < int(box_h))
    return;

  box.left = box.right - int(graph_w);
  box.bottom = box.top + int(box_h);

#ifdef ENABLE_OPENGL
  {
    const ScopeAlphaBlend blend;
    canvas.DrawFilledRectangle(box,
                               ColorWithAlpha(COLOR_BLACK, 0xb0));
  }
#else
  canvas.DrawFilledRectangle(box, COLOR_BLACK);
#endif

  PixelRect cpu_rc = box;
  cpu_rc.Grow(-int(Layout::Scale(2)));
  cpu_rc.bottom = cpu_rc.top + int(graph_h);
  PixelRect gpu_rc = cpu_rc;
  gpu_rc.top = cpu_rc.bottom;
  gpu_rc.bottom = gpu_rc.top + int(graph_h);

  const Pen cpu_pen(Layout::ScaleFinePenWidth(1), COLOR_CYAN);
  const Pen gpu_pen(Layout::ScaleFinePenWidth(1), COLOR_AMBER);
  DrawSparkline(canvas, cpu_rc, load_history.cpu_pct.data(),
                load_history.n, load_history.i, 200, cpu_pen);
  DrawSparkline(canvas, gpu_rc, load_history.gpu_ms.data(),
                load_history.n, load_history.i, 33, gpu_pen);

#ifdef ENABLE_OPENGL
  const unsigned polys = GetLastTopographyPolygonCount();
  const unsigned verts = GetLastTopographyVertexCount();
#else
  const unsigned polys = 0;
  const unsigned verts = 0;
#endif

  canvas.Select(*look.overlay.overlay_font);
  canvas.SetTextColor(COLOR_WHITE);
  canvas.SetBackgroundTransparent();

  const auto line1 = fmt::format("CPU {}%  GPU {}ms", cpu, frame_ms);
  const auto line2 = fmt::format("poly {}  vtx {}", polys, verts);
  canvas.DrawText({box.left + Layout::Scale(2), gpu_rc.bottom},
                  line1);
  canvas.DrawText({box.left + Layout::Scale(2),
                   gpu_rc.bottom + int(text_h)},
                  line2);
#ifdef ENABLE_OPENGL
  unsigned shown = 0;
  int y = gpu_rc.bottom + int(text_h) * 2;
  const auto *brk = GetLastTopographyLayerVtx();
  const unsigned n_brk = GetLastTopographyLayerVtxCount();
  for (unsigned i = 0; i < n_brk && shown < 4; ++i) {
    if (brk[i].vertices == 0)
      continue;
    const std::string_view full{brk[i].name};
    const auto line = fmt::format("{:.14} {}", full, brk[i].vertices);
    canvas.DrawText({box.left + Layout::Scale(2), y}, line);
    y += int(text_h);
    ++shown;
  }
#endif
}

#endif
