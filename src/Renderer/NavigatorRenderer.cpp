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
  utc_offset = CommonInterface::GetComputerSettings().utc_offset;

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
NavigatorRenderer::GenerateStringWaypointName(
    const Waypoint &wp_current) noexcept
{
  // waypoint_name_s; // e_WP_Name
  waypoint_name_s.Format(_T("%s"), wp_current.name.c_str());
}

void
NavigatorRenderer::GenerateStringsTimesInfo(const TaskType tp) noexcept
{
  // time_elapsed_s;
  if (tp == TaskType::ORDERED && has_started && basic->time_available) {
    const auto time_elapsed = TimeStamp{
        FloatDuration{calculated->ordered_task_stats.total.time_elapsed}};
    const BasicStringBuffer<TCHAR, 8> time_elapsed_s_tmp =
        FormatLocalTimeHHMM(time_elapsed, utc_no_offset);
    time_elapsed_s.Format(_T("%s"), time_elapsed_s_tmp.c_str());
  } else {
    time_elapsed_s.Format(_T("%s"), _T("--:--"));
  }

  // time_start_s;
  const auto time_start = calculated->ordered_task_stats.start.time;
  if (tp == TaskType::ORDERED && has_started && basic->time_available)
    time_start_s.Format(_T("%s"),
                        FormatLocalTimeHHMM(time_start, utc_offset).c_str());
  else {
    time_start_s.Format(_T("%s"), _T("--:--"));
  }

  // time_local_s;
  time_local_s.clear();
  if (basic->time_available) {
    const BasicStringBuffer<TCHAR, 8> time =
        FormatLocalTimeHHMM(basic->time, utc_offset);
    time_local_s.AppendFormat(_T("%s"), time.c_str());
  } else {
    time_local_s.Format(_T("%s"), _T("--:--"));
  }

  // time_planned_s;
  TimeStamp time_planned{};
  if (tp == TaskType::ORDERED && has_started && basic->time_available) {
    time_planned = TimeStamp{
        FloatDuration{calculated->ordered_task_stats.total.time_planned}};
    time_planned_s.Format(
        _T("%s"), FormatLocalTimeHHMM(time_planned, utc_no_offset).c_str());
  } else if (tp != TaskType::ORDERED && basic->time_available) {
    time_planned =
        TimeStamp{FloatDuration{calculated->task_stats.total.time_planned}};
    time_planned_s.Format(
        _T("%s"), FormatLocalTimeHHMM(time_planned, utc_no_offset).c_str());
  } else {
    time_planned_s.Format(_T("%s"), _T("--:--"));
  }

  // arrival_planned_s;
  TimeStamp arrival_planned{};
  if ((tp == TaskType::ORDERED && has_started) && basic->time_available) {
    arrival_planned = TimeStamp{
        FloatDuration{time_start.ToDuration() + time_planned.ToDuration()}};
    arrival_planned_s.Format(
        _T("%s"), FormatLocalTimeHHMM(arrival_planned, utc_offset).c_str());
  } else if (tp != TaskType::ORDERED && basic->time_available) {
    arrival_planned = TimeStamp{
        FloatDuration{basic->time.ToDuration() + time_planned.ToDuration()}};
    arrival_planned_s.Format(
        _T("%s"), FormatLocalTimeHHMM(arrival_planned, utc_offset).c_str());
  } else {
    arrival_planned_s.Format(_T("%s"), _T("--:--"));
  }

  // times_local_elapsed_s;
  times_local_elapsed_s.Format(_T("%s (%s)"), time_local_s.c_str(),
                               time_elapsed_s.c_str());

  // times_arrival_planned_s;
  if (canvas_width > canvas_height * 2.8) {
    times_arrival_planned_s.Format(_T("%s (%s)"), arrival_planned_s.c_str(),
                                   time_planned_s.c_str());
  } else {
    times_arrival_planned_s.Format(_T("%s"), arrival_planned_s.c_str());
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
NavigatorRenderer::DrawWaypointName(Canvas &canvas,
                                    const enum navType nav_type) noexcept
{
  switch (nav_type) {
  case navType::NAVIGATOR_LITE_ONE_LINE:
    font_height = canvas_height * 85 / 200;
    break;

  case navType::NAVIGATOR_LITE_TWO_LINES:
  case navType::NAVIGATOR:
    font_height = canvas_height * 75 / 200;
    break;

  case navType::NAVIGATOR_DETAILED:
  default:
    if (canvas_width > canvas_height * 3.7) {
      font_height = canvas_height * 42 / 200;
    } else {
      font_height = canvas_height * 30 / 200;
    }
    break;
  }

  font.Load(FontDescription(Layout::VptScale(font_height * ratio_dpi)));
  canvas.Select(font);

  sz_waypoint_name = canvas.CalcTextSize(waypoint_name_s).width;

  switch (nav_type) {
  case navType::NAVIGATOR_LITE_ONE_LINE:
    pos_x_waypoint_name = static_cast<int>(canvas_width * 13 / 200);
    pos_y_waypoint_name = static_cast<int>(canvas_height * 15 / 100);

    pos_x_end_waypoint_name = pos_x_waypoint_name + sz_waypoint_name;
    ps_drawed_text_size = {pxpt_pos_infos_waypoint.x -
                               static_cast<int>(canvas_height * 95 / 100),
                           static_cast<int>(canvas_height)};
    break;

  case navType::NAVIGATOR_LITE_TWO_LINES:
    pos_x_waypoint_name = canvas_width * 13 / 200;
    pos_y_waypoint_name = canvas_height * 38 / 100;

    sz_waypoint_name =
        std::min(sz_waypoint_name, canvas_width - 2 * pos_x_waypoint_name);

    pos_x_waypoint_name =
        (canvas_width - 2 * pos_x_waypoint_name - sz_waypoint_name) / 2 +
        canvas_width * 13 / 200;

    ps_drawed_text_size = {
        static_cast<unsigned int>(pos_x_waypoint_name + sz_waypoint_name),
        static_cast<unsigned>(font_height)};
    break;

  case navType::NAVIGATOR:
    pos_x_waypoint_name = static_cast<int>(canvas_width * 13 / 200);
    pos_y_waypoint_name = static_cast<int>(canvas_height * 38 / 100);

    pos_x_end_waypoint_name = pos_x_waypoint_name + sz_waypoint_name;
    ps_drawed_text_size = {static_cast<int>(canvas_height / 2 +
                                            pos_x_speed_altitude -
                                            canvas_height * 130 / 100),
                           static_cast<int>(canvas_height)};
    break;

  case navType::NAVIGATOR_DETAILED:
  default:
    pos_x_waypoint_name = pxpt_pos_infos_waypoint.x;
    pos_y_waypoint_name = static_cast<int>(canvas_height * 31 / 100);

    pos_x_end_waypoint_name = pos_x_waypoint_name + sz_waypoint_name;
    ps_drawed_text_size = {static_cast<int>(canvas_height) / 2 +
                               pos_x_speed_altitude -
                               static_cast<int>(canvas_height),
                           static_cast<int>(canvas_height)};
    break;
  }

  pp_drawed_text_origin = {0, 0};
  pr_drawed_text_rect = {pp_drawed_text_origin, ps_drawed_text_size};

  canvas.DrawClippedText({static_cast<int>(pos_x_waypoint_name),
                          static_cast<int>(pos_y_waypoint_name)},
                         pr_drawed_text_rect, waypoint_name_s);
}

void
NavigatorRenderer::DrawTimesInfo(Canvas &canvas, const PixelRect &rc,
                                 const enum navType nav_type) noexcept
{
  if (nav_type == navType::NAVIGATOR_DETAILED) {
    if (canvas_width > canvas_height * 3.6) {
      font.Load(FontDescription(
          Layout::VptScale(canvas_height * ratio_dpi * 32 / 200)));
    } else {
      font.Load(FontDescription(
          Layout::VptScale(canvas_height * ratio_dpi * 24 / 200)));
    }

    size_text = canvas.CalcTextSize(times_local_elapsed_s.c_str());
    const int pos_y_text_times{
        static_cast<int>(canvas_height * 183 / 200 - size_text.height)};
    const PixelRect pxrect_sz_time_start_s{rc.BottomAligned(canvas_width)};

    // Draw text start time -----------------------------------------
    const PixelPoint pxpt_pos_time_start_s{
        static_cast<int>(canvas_width * 5 / 200 + 8), pos_y_text_times};
    canvas.DrawClippedText(pxpt_pos_time_start_s, pxrect_sz_time_start_s,
                           time_start_s);

    // Draw text current time / elapsed time ------------------------
    const PixelRect pxlrect_sz_time_elapsed_s{rc.BottomAligned(canvas_width)};
    int size_text_tmp{};
    if (canvas_width > canvas_height * 2.8) {
      size_text_tmp = size_text.width * 3 / 4;
    } else {
      size_text_tmp = size_text.width / 2;
    }
    const PixelPoint pxpt_pos_time_elapsed_s{
        static_cast<int>(canvas_width * 100 / 200 - size_text_tmp),
        pos_y_text_times};
    canvas.DrawClippedText(pxpt_pos_time_elapsed_s, pxlrect_sz_time_elapsed_s,
                           times_local_elapsed_s);

    // Draw text planned time ---------------------------------------
    size_text = canvas.CalcTextSize(times_arrival_planned_s.c_str());
    const PixelRect pxlrect_sz_arrival_planned_s{
        rc.BottomAligned(canvas_width)};
    const PixelPoint pxpt_times_pos_arrival_planned_s{
        static_cast<int>(canvas_width - (canvas_width * 5 / 200 + 8) -
                         size_text.width),
        pos_y_text_times};
    canvas.DrawClippedText(pxpt_times_pos_arrival_planned_s,
                           pxlrect_sz_arrival_planned_s,
                           times_arrival_planned_s);
  }
}

void
NavigatorRenderer::DrawDirectionArrowNorthAnnulus(
    Canvas &canvas, const enum navType nav_type,
    const TaskLook &look_task) noexcept
{
  // Draw direction arrow / North direction -------------------------
  if (nav_type == navType::NAVIGATOR_LITE_ONE_LINE ||
      nav_type == navType::NAVIGATOR_DETAILED ||
      nav_type == navType::NAVIGATOR) {

    int pos_x_arrow{};
    int pos_y_arrow_offset{};
    int scale__arrow{};
    int pos_y_annulus{};
    int small_radius_annulus{};
    int big_radius_annulus{};
    int sz_max_waypoint_text{};

    const auto has_finished = calculated->ordered_task_stats.task_finished;
    if (!has_started) {
      canvas.Select(look_task.hbLightGray);
    } else {
      if (!has_finished) {
        canvas.Select(look_task.hbOrange);
      } else {
        canvas.Select(look_task.hbGreen);
      }
    }

    switch (nav_type) {
    case navType::NAVIGATOR_LITE_ONE_LINE:
      pos_x_arrow = pxpt_pos_infos_waypoint.x -
                    static_cast<int>(canvas_height * 95 / 100);
      pos_y_arrow_offset = canvas_height * 43 / 200;
      pos_y_annulus = static_cast<int>(canvas_height * 48 / 100);
      small_radius_annulus = static_cast<int>(canvas_height) * 26 / 100;
      big_radius_annulus = static_cast<int>(canvas_height * 40 / 100);
      scale__arrow = 14;

      if (static_cast<int>(sz_waypoint_name) <
          pos_x_arrow - static_cast<int>(canvas_height * 60 / 100) / 2) {
        pos_x_arrow = (pxpt_pos_infos_waypoint.x + sz_waypoint_name) / 2 -
                      static_cast<int>(canvas_height * 50 / 100) / 2;
      }
      break;

    case navType::NAVIGATOR:
      pos_x_arrow =
          pos_x_speed_altitude - static_cast<int>(canvas_height * 90 / 100);
      pos_y_arrow_offset = canvas_height * 43 / 200;
      pos_y_annulus = static_cast<int>(canvas_height * 48 / 100);
      small_radius_annulus = static_cast<int>(canvas_height) * 28 / 100;
      big_radius_annulus = static_cast<int>(canvas_height) * 36 / 100;
      scale__arrow = 14;
      sz_max_waypoint_text = static_cast<int>(
          std::max(pos_x_end_waypoint_name,
                   text_size_infos_waypoint + pxpt_pos_infos_waypoint.x));
      break;

    default:
      pos_x_arrow =
          pos_x_speed_altitude - static_cast<int>(canvas_height * 77 / 100);
      pos_y_arrow_offset = canvas_height * 13 / 100;
      pos_y_annulus = static_cast<int>(canvas_height * 75 / 200);
      small_radius_annulus = static_cast<int>(canvas_height) * 18 / 100;
      big_radius_annulus = static_cast<int>(canvas_height) * 26 / 100;
      scale__arrow = 21;
      sz_max_waypoint_text = static_cast<int>(
          std::max(pos_x_end_waypoint_name,
                   text_size_infos_waypoint + pxpt_pos_infos_waypoint.x));

      if (sz_max_waypoint_text <
              pos_x_speed_altitude - big_radius_annulus * 2 &&
          canvas_width > canvas_height * 4.2) {
        pos_x_arrow = (pos_x_speed_altitude + sz_max_waypoint_text) / 2 -
                      2 * big_radius_annulus;
      }
      break;
    }

    canvas.DrawAnnulus(
        {static_cast<int>(canvas_height) / 2 + pos_x_arrow, pos_y_annulus},
        small_radius_annulus, big_radius_annulus,
        -basic->track + Angle::Degrees(50),
        -basic->track + Angle::Degrees(310));

    canvas.DrawAnnulus(
        {static_cast<int>(canvas_height) / 2 + pos_x_arrow, pos_y_annulus},
        small_radius_annulus, big_radius_annulus,
        -basic->track - Angle::Degrees(8), -basic->track + Angle::Degrees(8));

    NextArrowRenderer next_arrow{UIGlobals::GetLook().next_arrow_info_box};
    pp_drawed_text_origin = {0, -static_cast<int>(canvas_height / 4)};
    ps_drawed_text_size = {static_cast<int>(canvas_height),
                           static_cast<int>(canvas_height)};
    PixelRect pixelrect_next_arrow{pp_drawed_text_origin, ps_drawed_text_size};
    pixelrect_next_arrow.Offset(pos_x_arrow, pos_y_arrow_offset);

    next_arrow.DrawArrowScale(canvas, pixelrect_next_arrow, bearing_diff,
                              scale__arrow);
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

  GenerateStringWaypointName(wp_current);

  GenerateStringsTimesInfo(tp);

  // Draw all Strings -----------------------------------------------
  SetTextColor(canvas, look_nav);

  DrawWaypointInfos(canvas, nav_type, look_infobox);

  DrawCurrentFlightInfos(canvas, nav_type, look_infobox);

  DrawWaypointName(canvas, nav_type);

  DrawTimesInfo(canvas, rc, nav_type);

  DrawDirectionArrowNorthAnnulus(canvas, nav_type, look_task);
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
