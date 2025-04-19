// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NavigatorRenderer.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Formatter/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Gauge/NavigatorWidget.hpp"
#include "Interface.hpp"
#include "Look/FontDescription.hpp"
#include "Look/InfoBoxLook.hpp"
#include "Look/Look.hpp"
#include "Look/MapLook.hpp"
#include "Look/NavigatorLook.hpp"
#include "Look/WaypointLook.hpp"
#include "Math/Angle.hpp"
#include "NextArrowRenderer.hpp"
#include "PageActions.hpp"
#include "PageSettings.hpp"
#include "ProgressBarRenderer.hpp"
#include "Renderer/TextRenderer.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "UnitSymbolRenderer.hpp"
#include "Waypoint/Waypoint.hpp"
#include "WaypointIconRenderer.hpp"
#include "WaypointRenderer.hpp"
#include "WindArrowRenderer.hpp"
#include "time/RoughTime.hpp"
#include "time/Stamp.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "util/StaticString.hxx"

// standard
#include <cmath>

void
NavigatorRenderer::Update(const Canvas &canvas) noexcept
{
  if (canvas_width != canvas.GetWidth() ||
      canvas_height != canvas.GetHeight()) {
    canvas_width = canvas.GetWidth();
    canvas_height = canvas.GetHeight();
    hasCanvasSizeChanged = true;
  } else {
    hasCanvasSizeChanged = false;
  }

  basic = &CommonInterface::Basic();
  calculated = &CommonInterface::Calculated();

  has_started = calculated->ordered_task_stats.start.HasStarted();

  ratio_dpi = 1.0 / Layout::vdpi * 100;
}

void
NavigatorRenderer::GenerateFrame(const PixelRect &rc,
                                 const bool is_frame_main) noexcept
{
  const int rc_height = rc.GetHeight();
  const int rc_width = rc.GetWidth();
  const auto rc_top_left = rc.GetTopLeft();

  /* the divisions below are not simplified deliberately to understand frame
     construction */
  const PixelPoint pt0 = {rc_top_left.x + rc_height * 5 / 20, rc_top_left.y};
  const PixelPoint pt1 = {rc_top_left.x + rc_width - rc_height * 4 / 20,
                          pt0.y};
  const PixelPoint pt2 = {rc_top_left.x + rc_width - rc_height * 2 / 20,
                          rc_top_left.y + rc_height * 1 / 20};
  const PixelPoint pt3 = {rc_top_left.x + rc_width,
                          rc_top_left.y + rc_height * 10 / 20};
  const PixelPoint pt4 = {pt2.x, rc_top_left.y + rc_height * 19 / 20};
  const PixelPoint pt5 = {pt1.x, rc_top_left.y + rc_height};
  const PixelPoint pt6 = {pt0.x, pt5.y};
  const PixelPoint pt7 = {rc_top_left.x + rc_height * 2 / 20, pt4.y};
  const PixelPoint pt8 = {rc_top_left.x, pt3.y};
  const PixelPoint pt9 = {pt7.x, pt2.y};

  if (is_frame_main) {
    polygone_frame_main = {pt0, pt1, pt2, pt3, pt4, pt5, pt6, pt7, pt8, pt9};
  } else {
    polygone_frame_detailed = {pt0, pt1, pt2, pt3, pt4,
                               pt5, pt6, pt7, pt8, pt9};
  }
}

void
NavigatorRenderer::DrawFrame(Canvas &canvas, const PixelRect &rc,
                             const NavigatorLook &look_nav,
                             const bool is_frame_main) noexcept
{
  if (hasCanvasSizeChanged) {
    GenerateFrame(rc, is_frame_main);
  }

  canvas.Select(look_nav.pen_frame);
  canvas.Select(look_nav.brush_frame);

  if (is_frame_main) {
    canvas.DrawPolygon(polygone_frame_main.data(), polygone_frame_main.size());
  } else {
    canvas.DrawPolygon(polygone_frame_detailed.data(),
                       polygone_frame_detailed.size());
  }
}

void
NavigatorRenderer::GenerateStringsWaypointInfos(const enum navType nav_type,
                                                const TaskType tp) noexcept
{
  // waypoint_distance_s; // e_WP_Distance
  auto precision_waypoint_distance{0};
  auto waypoint_distance{.0};

  if (tp == TaskType::ORDERED) {
    waypoint_distance =
        calculated->ordered_task_stats.current_leg.vector_remaining.distance;
  } else {
    waypoint_distance =
        calculated->task_stats.current_leg.vector_remaining.distance;
  }

  if (waypoint_distance < 5000.0) {
    precision_waypoint_distance = 1;
  }

  FormatUserDistance(waypoint_distance, waypoint_distance_s.data(), false,
                     precision_waypoint_distance);

  // waypoint_altitude_s (configurable choice: WP_AltReq WP_AltDiff
  // WP_AltArrival)
  auto waypoint_altitude{.0};
  const auto &navigatorAltitudeType =
      CommonInterface::GetUISettings().navigator.navigator_altitude_type;

  if (tp == TaskType::ORDERED) {
    switch (navigatorAltitudeType) {
    case NavigatorSettings::NavigatorWidgetAltitudeType::WP_AltReq:
      waypoint_altitude = calculated->ordered_task_stats.current_leg
                              .solution_remaining.GetRequiredAltitude();
      break;
    case NavigatorSettings::NavigatorWidgetAltitudeType::WP_AltDiff:
      waypoint_altitude = calculated->ordered_task_stats.current_leg
                              .solution_remaining.altitude_difference;
      break;
    case NavigatorSettings::NavigatorWidgetAltitudeType::WP_AltArrival:
      waypoint_altitude = calculated->ordered_task_stats.current_leg
                              .solution_remaining.GetArrivalAltitude();
      break;
    default:
      waypoint_altitude = calculated->ordered_task_stats.current_leg
                              .solution_remaining.GetRequiredAltitude();
      break;
    }
  } else {
    switch (navigatorAltitudeType) {
    case NavigatorSettings::NavigatorWidgetAltitudeType::WP_AltReq:
      waypoint_altitude = calculated->task_stats.current_leg.solution_remaining
                              .GetRequiredAltitude();
      break;
    case NavigatorSettings::NavigatorWidgetAltitudeType::WP_AltDiff:
      waypoint_altitude = calculated->task_stats.current_leg.solution_remaining
                              .altitude_difference;
      break;
    case NavigatorSettings::NavigatorWidgetAltitudeType::WP_AltArrival:
      waypoint_altitude = calculated->task_stats.current_leg.solution_remaining
                              .GetArrivalAltitude();
      break;
    default:
      waypoint_altitude = calculated->task_stats.current_leg.solution_remaining
                              .GetRequiredAltitude();
      break;
    }
  }

  if (navigatorAltitudeType !=
      NavigatorSettings::NavigatorWidgetAltitudeType::WP_AltDiff) {
    FormatAltitude(waypoint_altitude_s.data(), waypoint_altitude,
                   Units::GetUserAltitudeUnit(), false);
  } else {
    FormatRelativeAltitude(waypoint_altitude_s.data(), waypoint_altitude,
                           Units::GetUserAltitudeUnit(), false);
  }

  // waypoint_GR_s; // e_WP_GR glide ratio
  auto waypoint_GR{0};
  if (tp == TaskType::ORDERED)
    waypoint_GR =
        std::round(calculated->ordered_task_stats.current_leg.gradient);
  else {
    waypoint_GR = std::round(calculated->task_stats.current_leg.gradient);
  }
  waypoint_GR_s.Format(_T("%d"), waypoint_GR);

  // waypoint_direction_s; // e_WP_BearingDiff
  if (!basic->track_available) {
    bearing_diff.Zero();
  } else if (tp == TaskType::ORDERED)
    bearing_diff =
        calculated->ordered_task_stats.current_leg.vector_remaining.bearing -
        basic->track;
  else {
    bearing_diff =
        calculated->task_stats.current_leg.vector_remaining.bearing -
        basic->track;
  }
  const int waypoint_direction = std::round(bearing_diff.AsDelta().Degrees());
  waypoint_direction_s.Format(_T("%d°"), waypoint_direction);

  // infos_next_waypoint_s; // dist + alt + GR
  switch (nav_type) {
  case navType::NAVIGATOR_LITE_ONE_LINE:
    infos_waypoint_s.Format(_T("%s   %s"), waypoint_distance_s.c_str(),
                            waypoint_altitude_s.c_str());
    break;

  case navType::NAVIGATOR_LITE_TWO_LINES:
    infos_waypoint_s.Format(
        _T("%s       %s       %s"), waypoint_distance_s.c_str(),
        waypoint_altitude_s.c_str(), waypoint_GR_s.c_str());
    break;

  case navType::NAVIGATOR:
    if (canvas_width > canvas_height * 8.1) {
      infos_waypoint_s.Format(
          _T("%s   %s  %s  %s"), waypoint_distance_s.c_str(),
          waypoint_altitude_s.c_str(), waypoint_GR_s.c_str(),
          waypoint_direction_s.c_str());
    } else if (canvas_width > canvas_height * 4.9) {
      infos_waypoint_s.Format(_T("%s   %s  %s"), waypoint_distance_s.c_str(),
                              waypoint_altitude_s.c_str(),
                              waypoint_GR_s.c_str());
    } else {
      infos_waypoint_s.Format(_T("%s   %s"), waypoint_distance_s.c_str(),
                              waypoint_altitude_s.c_str());
    }
    break;
  case navType::NAVIGATOR_DETAILED:
  default:
    if (canvas_width > canvas_height * 8.1) {
      infos_waypoint_s.Format(
          _T("%s   %s  %s  %s"), waypoint_distance_s.c_str(),
          waypoint_altitude_s.c_str(), waypoint_GR_s.c_str(),
          waypoint_direction_s.c_str());
    } else if (canvas_width > canvas_height * 2.9) {
      infos_waypoint_s.Format(_T("%s   %s  %s"), waypoint_distance_s.c_str(),
                              waypoint_altitude_s.c_str(),
                              waypoint_GR_s.c_str());
    } else {
      infos_waypoint_s.Format(_T("%s   %s"), waypoint_distance_s.c_str(),
                              waypoint_altitude_s.c_str());
    }
    break;
  }

  // infos_next_waypoint_units__dist_alt_s;
  if (nav_type != navType::NAVIGATOR_LITE_TWO_LINES) {
    infos_waypoint_units_dist_alt_s.Format(_T("%s   %s"),
                                           waypoint_distance_s.c_str(),
                                           waypoint_altitude_s.c_str());
  } else {
    infos_waypoint_units_dist_alt_s.Format(_T("%s       %s"),
                                           waypoint_distance_s.c_str(),
                                           waypoint_altitude_s.c_str());
  }

  // infos_next_waypoint_units__dist_alt_GR_s;
  if (nav_type != navType::NAVIGATOR_LITE_TWO_LINES) {
    infos_waypoint_units_dist_alt_GR_s.Format(
        _T("%s   %s  %s"), waypoint_distance_s.c_str(),
        waypoint_altitude_s.c_str(), waypoint_GR_s.c_str());
  } else {
    infos_waypoint_units_dist_alt_GR_s.Format(
        _T("%s       %s       %s"), waypoint_distance_s.c_str(),
        waypoint_altitude_s.c_str(), waypoint_GR_s.c_str());
  }
}

void
NavigatorRenderer::GenerateStringsCurrentFlightInfo(const TaskType tp) noexcept
{
  // current_speed_s; // e_Speed_GPS
  FormatUserSpeed(basic->ground_speed, current_speed_s.data(), false, 0);

  // current_altitude_s; // e_HeightGPS
  FormatUserAltitude(basic->gps_altitude, current_altitude_s.data(), false);

  // waypoint_average_speed_s; // e_SpeedTaskAvg
  if (tp == TaskType::ORDERED && has_started) {
    FormatUserSpeed(calculated->task_stats.total.travelled.GetSpeed(),
                    waypoint_average_speed_s.data(), false, 0);
  } else {
    waypoint_average_speed_s.Format(_T("%s"), _T("---"));
  }
}
void
NavigatorRenderer::SetTextColor(Canvas &canvas,
                                const NavigatorLook &look_nav) noexcept
{
  canvas.SetBackgroundTransparent();
  if (!look_nav.inverse) {
    canvas.SetTextColor(COLOR_BLACK);
  } else {
    canvas.SetTextColor(COLOR_WHITE);
  }
}

void
NavigatorRenderer::DrawWaypointInfos(Canvas &canvas,
                                     const enum navType nav_type,
                                     const InfoBoxLook &look_infobox) noexcept
{
  switch (nav_type) {
  case navType::NAVIGATOR_LITE_ONE_LINE:
    font_height = canvas_height * 85 / 200;
    break;

  case navType::NAVIGATOR_LITE_TWO_LINES:
  case navType::NAVIGATOR:
    font_height = canvas_height * 63 / 200;
    break;

  case navType::NAVIGATOR_DETAILED:
  default:
    if (canvas_width > canvas_height * 3.7)
      font_height = canvas_height * 35 / 200;
    else {
      font_height = canvas_height * 26 / 200;
    }
    break;
  }

  font.Load(FontDescription(Layout::VptScale(font_height * ratio_dpi)));
  canvas.Select(font);

  text_size_infos_waypoint =
      canvas.CalcTextSize(infos_waypoint_s.c_str()).width;

  switch (nav_type) {
  case navType::NAVIGATOR_LITE_ONE_LINE:
    pxpt_pos_infos_waypoint = {static_cast<int>(canvas_width * 96 / 100 -
                                                text_size_infos_waypoint -
                                                canvas_height * 40 / 100),
                               static_cast<int>(canvas_height * 15 / 100)};
    break;

  case navType::NAVIGATOR_LITE_TWO_LINES:
    pxpt_pos_infos_waypoint = {
        static_cast<int>(
            (canvas_width - 2 * canvas_width * 13 / 200 -
             canvas.CalcTextSize(infos_waypoint_s.c_str()).width) /
                2 +
            canvas_width * 13 / 200),
        static_cast<int>(canvas_height * 2 / 100)};
    break;

  case navType::NAVIGATOR:
    pxpt_pos_infos_waypoint = {static_cast<int>(canvas_width * 13 / 200),
                               static_cast<int>(canvas_height * 2 / 100)};
    break;

  case navType::NAVIGATOR_DETAILED:
  default:
    pxpt_pos_infos_waypoint = {static_cast<int>(canvas_width * 40 / 200),
                               static_cast<int>(canvas_height * 10 / 100)};
    break;
  }

  pp_drawed_text_origin = {0, 0};
  ps_drawed_text_size = {static_cast<int>(canvas_width),
                         static_cast<int>(canvas_height)};
  pr_drawed_text_rect = {pp_drawed_text_origin, ps_drawed_text_size};
  canvas.DrawClippedText(pxpt_pos_infos_waypoint, pr_drawed_text_rect,
                         infos_waypoint_s);

  // Draw Waypoint distance unit ------------------------------------
  unit = Units::GetUserDistanceUnit();
  unit_height = static_cast<unsigned int>(font_height * 4 / 10);
  font.Load(FontDescription(Layout::VptScale(font_height * ratio_dpi)));
  size_text = canvas.CalcTextSize(waypoint_distance_s.c_str());
  font.Load(FontDescription(Layout::VptScale(unit_height * ratio_dpi)));
  ascent_height = UnitSymbolRenderer::GetAscentHeight(font, unit);
  pp_pos_unit = pxpt_pos_infos_waypoint.At(
      size_text.width, size_text.height - ascent_height * 1.6);
  canvas.Select(font);
  UnitSymbolRenderer::Draw(canvas, pp_pos_unit, unit,
                           look_infobox.unit_fraction_pen);

  // Draw Waypoint altitude unit ------------------------------------
  unit = Units::GetUserAltitudeUnit();
  font.Load(FontDescription(Layout::VptScale(font_height * ratio_dpi)));
  size_text = canvas.CalcTextSize(infos_waypoint_units_dist_alt_s.c_str());
  font.Load(FontDescription(Layout::VptScale(unit_height * ratio_dpi)));
  ascent_height = UnitSymbolRenderer::GetAscentHeight(font, unit);
  pp_pos_unit = pxpt_pos_infos_waypoint.At(
      size_text.width, size_text.height - ascent_height * 1.6);
  canvas.Select(font);
  UnitSymbolRenderer::Draw(canvas, pp_pos_unit, unit,
                           look_infobox.unit_fraction_pen);

  // Draw Waypoint GR unit (glide ratio) ----------------------------
  if ((canvas_width > canvas_height * 2.9 &&
       nav_type == navType::NAVIGATOR_LITE_TWO_LINES) ||
      (canvas_width > canvas_height * 4.9 && nav_type == navType::NAVIGATOR) ||
      (canvas_width > canvas_height * 2.9 &&
       nav_type == navType::NAVIGATOR_DETAILED)) {
    unit = Unit::GRADIENT;
    font.Load(FontDescription(Layout::VptScale(font_height * ratio_dpi)));
    size_text =
        canvas.CalcTextSize(infos_waypoint_units_dist_alt_GR_s.c_str());
    font.Load(FontDescription(Layout::VptScale(unit_height * ratio_dpi)));
    ascent_height = UnitSymbolRenderer::GetAscentHeight(font, unit);
    pp_pos_unit = pxpt_pos_infos_waypoint.At(
        size_text.width, size_text.height - ascent_height * 1.6);
    canvas.Select(font);
    UnitSymbolRenderer::Draw(canvas, pp_pos_unit, unit,
                             look_infobox.unit_fraction_pen);
  }
}

void
NavigatorRenderer::DrawCurrentFlightInfos(
    Canvas &canvas, const enum navType nav_type,
    const InfoBoxLook &look_infobox) noexcept
{
  int pos_y_current_altitude{};
  int pos_y_current_speed{};

  if (canvas_width > canvas_height * 3.2) {
    nav_type == navType::NAVIGATOR ? font_height = canvas_height * 55 / 200
                                   : font_height = canvas_height * 40 / 200;
  } else {
    nav_type == navType::NAVIGATOR ? font_height = canvas_height * 55 / 200
                                   : font_height = canvas_height * 30 / 200;
  }

  font.Load(FontDescription(Layout::VptScale(font_height * ratio_dpi)));
  // grow artificially the current_speed_s string ('format tricks')
  StaticString<7> sz_tmp_current_speed_s;
  sz_tmp_current_speed_s.clear();
  sz_tmp_current_speed_s.append(_T("0"));
  sz_tmp_current_speed_s.append(current_speed_s);
  const auto text_size_current_speed_s =
      canvas.CalcTextSize(sz_tmp_current_speed_s.c_str());
  const auto text_size_current_altitude_s =
      canvas.CalcTextSize(current_altitude_s.c_str());

  switch (nav_type) {
  case navType::NAVIGATOR_LITE_ONE_LINE:
    size_text = canvas.CalcTextSize(infos_waypoint_s.c_str());
    pos_x_speed_altitude =
        canvas_width * 96 / 100 - size_text.width - canvas_height * 16 / 100;
    pos_y_current_speed = static_cast<int>(canvas_height * 10 / 100);
    pos_y_current_altitude = static_cast<int>(canvas_height * 35 / 100);
    break;

  case navType::NAVIGATOR:
  case navType::NAVIGATOR_LITE_TWO_LINES:
    size_text.width = std::max(text_size_current_speed_s.width,
                               text_size_current_altitude_s.width);
    pos_x_speed_altitude =
        canvas_width * 96 / 100 - size_text.width - canvas_height * 16 / 100;
    pos_y_current_speed = static_cast<int>(canvas_height * 5 / 100);
    pos_y_current_altitude = static_cast<int>(canvas_height * 45 / 100);
    break;

  case navType::NAVIGATOR_DETAILED:
  default:
    size_text.width = std::max(text_size_current_speed_s.width,
                               text_size_current_altitude_s.width);
    if (canvas_width > canvas_height * 3.2) {
      pos_x_speed_altitude =
          canvas_width * 96 / 100 - size_text.width - canvas_height * 16 / 100;
      pos_y_current_speed = static_cast<int>(canvas_height * 10 / 100);
      pos_y_current_altitude = static_cast<int>(canvas_height * 35 / 100);
    } else {
      pos_x_speed_altitude = canvas_width * 103 / 100 - size_text.width -
                             canvas_height * 29 / 100;
      pos_y_current_speed = static_cast<int>(canvas_height * 13 / 100);
      pos_y_current_altitude = static_cast<int>(canvas_height * 42 / 100);
    }
    break;
  }

  if ((nav_type == navType::NAVIGATOR_DETAILED ||
       nav_type == navType::NAVIGATOR) &&
      canvas_width > canvas_height * 2.3) {

    // Current speed ------------------------------------------------
    const PixelPoint pxpt_pos_current_speed{pos_x_speed_altitude,
                                            pos_y_current_speed};
    canvas.Select(font);
    pp_drawed_text_origin = {0, 0};
    ps_drawed_text_size = {static_cast<int>(canvas_width),
                           static_cast<int>(canvas_height)};
    pr_drawed_text_rect = {pp_drawed_text_origin, ps_drawed_text_size};
    canvas.DrawClippedText(pxpt_pos_current_speed, pr_drawed_text_rect,
                           current_speed_s);

    // Draw speed units ---------------------------------------------
    unit = Units::GetUserSpeedUnit();
    unit_height = static_cast<unsigned int>(font_height * 38 / 100);
    font.Load(FontDescription(Layout::VptScale(unit_height * ratio_dpi)));
    size_text = canvas.CalcTextSize(current_speed_s.c_str());
    pp_pos_unit =
        pxpt_pos_current_speed.At(size_text.width, size_text.height / 10);
    canvas.Select(font);
    UnitSymbolRenderer::Draw(canvas, pp_pos_unit, unit,
                             look_infobox.unit_fraction_pen);

    // Current Altitude --------------------------------------------
    font.Load(FontDescription(Layout::VptScale(font_height * ratio_dpi)));
    const PixelPoint pxpt_pos_altitude{pos_x_speed_altitude,
                                       pos_y_current_altitude};
    canvas.Select(font);
    ps_drawed_text_size = {static_cast<int>(canvas_width),
                           static_cast<int>(canvas_height)};
    pr_drawed_text_rect = {pp_drawed_text_origin, ps_drawed_text_size};
    canvas.DrawClippedText(pxpt_pos_altitude, pr_drawed_text_rect,
                           current_altitude_s);

    // Draw Altitude unit -------------------------------------------
    unit = Units::GetUserAltitudeUnit();
    unit_height = static_cast<unsigned int>(font_height * 0.5);
    font.Load(FontDescription(Layout::VptScale(unit_height * ratio_dpi)));
    size_text = canvas.CalcTextSize(current_altitude_s.c_str());
    pp_pos_unit =
        pxpt_pos_altitude.At(size_text.width, size_text.height * 53 / 100);
    UnitSymbolRenderer::Draw(canvas, pp_pos_unit, unit,
                             look_infobox.unit_fraction_pen);
  }
}

void
NavigatorRenderer::DrawTaskTextsArrow(
    Canvas &canvas, TaskType tp, [[maybe_unused]] const Waypoint &wp_current,
    [[maybe_unused]] const PixelRect &rc, const enum navType nav_type,
    [[maybe_unused]] const bool isNavTopPosition,
    const NavigatorLook &look_nav, [[maybe_unused]] const TaskLook &look_task,
    const InfoBoxLook &look_infobox) noexcept
{
  // Generate all strings -------------------------------------------
  GenerateStringsWaypointInfos(nav_type, tp);

  GenerateStringsCurrentFlightInfo(tp);

  // Draw all Strings -----------------------------------------------
  SetTextColor(canvas, look_nav);

  DrawWaypointInfos(canvas, nav_type, look_infobox);

  DrawCurrentFlightInfos(canvas, nav_type, look_infobox);
}

void
NavigatorRenderer::DrawProgressTask(const TaskSummary &summary, Canvas &canvas,
                                    const PixelRect &rc,
                                    const NavigatorLook &look_nav,
                                    const TaskLook &look_task) noexcept
{
  const int rc_height = rc.GetHeight();
  const int rc_width = rc.GetWidth();

  // render the progress bar
  PixelRect r{rc_height * 5 / 24, rc_height - rc_height * 5 / 48,
              rc_width - rc_height * 10 / 48, rc_height - rc_height * 1 / 48};

  bool task_has_started =
      CommonInterface::Calculated().task_stats.start.HasStarted();
  bool task_is_finished =
      CommonInterface::Calculated().task_stats.task_finished;

  unsigned int progression{};

  if (task_has_started && !task_is_finished)
    progression = 100 * (1 - summary.p_remaining);
  else if (task_has_started && task_is_finished) progression = 100;
  else progression = 0;

  DrawSimpleProgressBar(canvas, r, progression, 0, 100);

  canvas.Select(look_nav.brush_frame);

  // render the waypoints on the progress bar
  const Pen pen_waypoint(Layout::ScalePenWidth(1), COLOR_BLACK);
  const Pen pen_indications(Layout::ScalePenWidth(1),
                            look_nav.inverse ? COLOR_GRAY : COLOR_DARK_GRAY);
  canvas.Select(pen_indications);

  bool target{true};
  unsigned i = 0;
  for (auto it = summary.pts.begin(); it != summary.pts.end(); ++it, ++i) {
    auto p = it->p;

    const PixelPoint position_waypoint(
        p * (rc_width - 10 / 24.0 * rc_height) + 5 / 24.0 * rc_height,
        rc_height - static_cast<int>(1.5 / 24.0 * rc_height));

    int w = Layout::Scale(2);

    /* search for the next Waypoint to reach and draw two horizontal lines
     * left and right if one Waypoint has been missed, the two lines are also
     * drawn
     */
    if (!it->achieved && target) {
      canvas.Select(pen_indications);
      canvas.DrawLine(position_waypoint.At(-w, 0.5 * w),
                      position_waypoint.At(-2 * w, 0.5 * w));
      canvas.DrawLine(position_waypoint.At(w, 0.5 * w),
                      position_waypoint.At(2 * w, 0.5 * w));

      canvas.DrawLine(position_waypoint.At(-w, -0.5 * w),
                      position_waypoint.At(-2 * w, -0.5 * w));
      canvas.DrawLine(position_waypoint.At(w, -0.5 * w),
                      position_waypoint.At(2 * w, -0.5 * w));

      target = false;
    }

    if (i == summary.active) {
      // search for the Waypoint on which the user is looking for and draw
      // two vertical lines left and right
      canvas.Select(pen_indications);
      canvas.DrawLine(position_waypoint.At(-2 * w, w),
                      position_waypoint.At(-2 * w, -w));
      canvas.DrawLine(position_waypoint.At(2 * w, w),
                      position_waypoint.At(2 * w, -w));

      if (it->achieved) canvas.Select(look_task.hbGreen);
      else canvas.Select(look_task.hbOrange);
      w = Layout::Scale(2);
    } else if (i < summary.active) {
      if (it->achieved) canvas.Select(look_task.hbGreen);
      else canvas.Select(look_task.hbNotReachableTerrain);
      w = Layout::Scale(2);
    } else {
      if (it->achieved) canvas.Select(look_task.hbGreen);
      else canvas.Select(look_task.hbLightGray);

      w = Layout::Scale(1);
    }

    canvas.Select(pen_waypoint);
    canvas.DrawRectangle(PixelRect{position_waypoint}.WithMargin(w));
  }
}

void
NavigatorRenderer::DrawWaypointsIconsTitle(
    Canvas &canvas, WaypointPtr waypoint_before, WaypointPtr waypoint_current,
    unsigned task_size, [[maybe_unused]] const NavigatorLook &look,
    const enum navType nav_type) noexcept
{
  const int rc_height = canvas_height;

  const WaypointRendererSettings &waypoint_settings =
      CommonInterface::GetMapSettings().waypoint;
  const WaypointLook &waypoint_look = UIGlobals::GetMapLook().waypoint;

  WaypointIconRenderer waypoint_icon_renderer{waypoint_settings, waypoint_look,
                                              canvas};
  const PixelPoint position_waypoint_left{
      static_cast<int>(canvas_width * 7 / 200), rc_height * 1 / 2};

  static PixelPoint position_waypoint_right{};

  if (nav_type == navType::NAVIGATOR_DETAILED) {
    position_waypoint_right = {
        static_cast<int>(canvas_width * 98 / 100 - rc_height * 15 / 100),
        rc_height * 38 / 100};
  } else {
    position_waypoint_right = {static_cast<int>(canvas_width * 193 / 200),
                               rc_height * 1 / 2};
  }

  // CALCULATE REACHABILITY
  /// TODO: implement waypoint reachability
  /// without using costing calculation inside renderer
  WaypointReachability wr_before{WaypointReachability::UNREACHABLE};
  WaypointReachability wr_current{WaypointReachability::UNREACHABLE};

  auto *protected_task_manager =
      backend_components->protected_task_manager.get();
  if (protected_task_manager != nullptr && task_size > 1) {
    if (waypoint_before != nullptr)
      waypoint_icon_renderer.Draw(*waypoint_before, position_waypoint_left,
                                  wr_before, true);
    if (waypoint_current != nullptr)
      waypoint_icon_renderer.Draw(*waypoint_current, position_waypoint_right,
                                  wr_current, true);
  }
}
