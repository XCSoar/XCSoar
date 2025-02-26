// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NavigatorRenderer.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Formatter/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Look/FontDescription.hpp"
#include "Look/InfoBoxLook.hpp"
#include "Look/Look.hpp"
#include "Look/MapLook.hpp"
#include "Look/NavigatorLook.hpp"
#include "Look/WaypointLook.hpp"
#include "Math/Angle.hpp"
#include "NextArrowRenderer.hpp"
#include "ProgressBarRenderer.hpp"
#include "Renderer/TextRenderer.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "UnitSymbolRenderer.hpp"
#include "Waypoint/Waypoint.hpp"
#include "WaypointIconRenderer.hpp"
#include "WaypointRenderer.hpp"
#include "WindArrowRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "util/StaticString.hxx"

// standard
#include <cmath>

void
NavigatorRenderer::DrawFrame(Canvas &canvas, const PixelRect &rc,
                             const NavigatorLook &look_nav) noexcept
{
  const auto top_left = rc.GetTopLeft();
  const int rc_height = rc.GetHeight();
  const int rc_width = rc.GetWidth();

  /* the divisions below are not simplified deliberately to understand frame
     construction */
  const PixelPoint pt0 = {top_left.x + rc_height * 5 / 20, top_left.y};
  const PixelPoint pt1 = {top_left.x + rc_width - rc_height * 4 / 20, pt0.y};
  const PixelPoint pt2 = {top_left.x + rc_width - rc_height * 2 / 20,
                          top_left.y + rc_height * 1 / 20};
  const PixelPoint pt3 = {top_left.x + rc_width,
                          top_left.y + rc_height * 10 / 20};
  const PixelPoint pt4 = {pt2.x, top_left.y + rc_height * 19 / 20};
  const PixelPoint pt5 = {pt1.x, top_left.y + rc_height};
  const PixelPoint pt6 = {pt0.x, pt5.y};
  const PixelPoint pt7 = {top_left.x + rc_height * 2 / 20, pt4.y};
  const PixelPoint pt8 = {top_left.x, pt3.y};
  const PixelPoint pt9 = {pt7.x, pt2.y};

  const StaticArray<BulkPixelPoint, 10> polygone_frame = {
      pt0, pt1, pt2, pt3, pt4, pt5, pt6, pt7, pt8, pt9};

  canvas.Select(look_nav.pen_frame);
  canvas.Select(look_nav.brush_frame);
  canvas.DrawPolygon(polygone_frame.data(), polygone_frame.size());
}

void
NavigatorRenderer::DrawTaskText(
    Canvas &canvas, TaskType tp, [[maybe_unused]] const Waypoint &wp_current,
    const PixelRect &rc, [[maybe_unused]] const NavigatorLook &look_nav,
    [[maybe_unused]] const InfoBoxLook &look_infobox) noexcept
{
  const auto &basic = CommonInterface::Basic();
  const auto &calculated = CommonInterface::Calculated();

  bool has_started = calculated.ordered_task_stats.start.HasStarted();

  const int rc_width = rc.GetWidth();
  const int rc_height = rc.GetHeight();

  // e_WP_Distance
  static StaticString<20> waypoint_distance_s;
  auto precision_waypoint_distance{0};
  auto waypoint_distance{.0};
  if (tp == TaskType::ORDERED) {
    waypoint_distance =
        calculated.ordered_task_stats.current_leg.vector_remaining.distance;
  } else {
    waypoint_distance =
        calculated.task_stats.current_leg.vector_remaining.distance;
  }

  if (waypoint_distance < 5000.0) precision_waypoint_distance = 1;

  FormatUserDistance(waypoint_distance, waypoint_distance_s.data(), false,
                     precision_waypoint_distance);

  // e_WP_AltReq
  /// TODO: or e_WP_H ?
  static StaticString<20> waypoint_altitude_diff_s;
  auto waypoint_altitude_diff{.0};
  if (tp == TaskType::ORDERED) {
    waypoint_altitude_diff = calculated.ordered_task_stats.current_leg
                                 .solution_remaining.GetRequiredAltitude();
  } else {
    waypoint_altitude_diff = calculated.task_stats.current_leg
                                 .solution_remaining.GetRequiredAltitude();
  }
  FormatAltitude(waypoint_altitude_diff_s.data(), waypoint_altitude_diff,
                 Units::GetUserAltitudeUnit(), false);

  // e_SpeedTaskAvg
  static StaticString<20> waypoint_average_speed_s;
  if (tp == TaskType::ORDERED && has_started)
    FormatUserSpeed(calculated.task_stats.total.travelled.GetSpeed(),
                    waypoint_average_speed_s.data(), false, 0);
  else
    waypoint_average_speed_s.Format(_T("%s"), _T("---"));

  // e_WP_GR
  static StaticString<20> waypoint_direction_s;
  static StaticString<20> waypoint_GR_s;
  auto waypoint_GR{0};
  if (tp == TaskType::ORDERED)
    waypoint_GR =
      std::round(calculated.ordered_task_stats.current_leg.gradient);
  else
    waypoint_GR =
      std::round(calculated.task_stats.current_leg.gradient);
  waypoint_GR_s.Format(_T("%d"), waypoint_GR);

  // e_Speed_GPS
  static StaticString<20> current_speed_s;
  FormatUserSpeed(basic.ground_speed, current_speed_s.data(), false, 0);

  static StaticString<100> infos_next_waypoint_s;
  if (canvas.GetWidth() > canvas.GetHeight() * 6.1)
    infos_next_waypoint_s.Format(
        _T("%s   %s  %s  %s"), waypoint_distance_s.c_str(),
        waypoint_altitude_diff_s.c_str(), waypoint_GR_s.c_str(),
        waypoint_direction_s.c_str());
  else if (canvas.GetWidth() > canvas.GetHeight() * 2.9)
    infos_next_waypoint_s.Format(
        _T("%s   %s  %s"), waypoint_distance_s.c_str(),
        waypoint_altitude_diff_s.c_str(), waypoint_GR_s.c_str());
  else
    infos_next_waypoint_s.Format(_T("%s   %s"), waypoint_distance_s.c_str(),
                                 waypoint_altitude_diff_s.c_str());

  static StaticString<60> infos_next_waypoint_unit_alt_s;
  infos_next_waypoint_unit_alt_s.Format(_T("%s   %s"),
                                        waypoint_distance_s.c_str(),
                                        waypoint_altitude_diff_s.c_str());

  static StaticString<60> infos_next_waypoint_unit_GR_s;
  infos_next_waypoint_unit_GR_s.Format(
      _T("%s   %s  %s"), waypoint_distance_s.c_str(),
      waypoint_altitude_diff_s.c_str(), waypoint_GR_s.c_str());

  canvas.SetBackgroundTransparent();
  if (!look_nav.inverse)
    canvas.SetTextColor(COLOR_BLACK);
  else
    canvas.SetTextColor(COLOR_WHITE);

  // ---- Draw Waypoint informations: distance, altitude, glide ratio
  Font font;
  double ratio_dpi = 1.0 / Layout::vdpi * 100;
  unsigned int font_height{};
  if (canvas.GetWidth() > canvas.GetHeight() * 3.7)
    font_height = rc_height * 35 / 200;
  else
    font_height = rc_height * 26 / 200;
  PixelPoint pxpt_pos_infos_next_waypoint {
    static_cast<int>(rc_width * 40 / 200),
    static_cast<int>(rc_height * 10 / 100)
  };
  font.Load(FontDescription(Layout::VptScale(font_height * ratio_dpi)));
  canvas.Select(font);

  PixelPoint ppOrigin{0, 0};
  PixelSize psSize{static_cast<int>(rc_width * 2 / 3),
                   static_cast<int>(rc_height)};
  PixelRect prRect{ppOrigin, psSize};
  canvas.DrawClippedText(pxpt_pos_infos_next_waypoint, prRect,
                         infos_next_waypoint_s);

  // Draw Waypoint units
  PixelSize size_text{};
  unsigned int unit_height{};
  unsigned int ascent_height{};
  PixelPoint unit_p{};
  Unit unit{Unit::KILOMETER};
  unit = Units::GetUserDistanceUnit();
  unit_height = static_cast<unsigned int>(font_height * 4 / 10);
  size_text = canvas.CalcTextSize(waypoint_distance_s.c_str());
  font.Load(FontDescription(Layout::VptScale(unit_height * ratio_dpi)));
  ascent_height = UnitSymbolRenderer::GetAscentHeight(font, unit);
  unit_p = pxpt_pos_infos_next_waypoint.At(
      size_text.width, size_text.height - ascent_height * 1.6);
  canvas.Select(font);
  UnitSymbolRenderer::Draw(canvas, unit_p, unit,
                           look_infobox.unit_fraction_pen);

  // Draw Waypoint units
  unit = Units::GetUserAltitudeUnit();
  font.Load(FontDescription(Layout::VptScale(font_height * ratio_dpi)));
  size_text = canvas.CalcTextSize(infos_next_waypoint_unit_alt_s.c_str());
  font.Load(FontDescription(Layout::VptScale(unit_height * ratio_dpi)));
  ascent_height = UnitSymbolRenderer::GetAscentHeight(font, unit);
  unit_p = pxpt_pos_infos_next_waypoint.At(
      size_text.width, size_text.height - ascent_height * 1.6);
  canvas.Select(font);
  UnitSymbolRenderer::Draw(canvas, unit_p, unit,
                           look_infobox.unit_fraction_pen);

  // Draw GR units
  if (canvas.GetWidth() > canvas.GetHeight() * 2.9) {
    unit = Unit::GRADIENT;
    font.Load(FontDescription(Layout::VptScale(font_height * ratio_dpi)));
    size_text = canvas.CalcTextSize(infos_next_waypoint_unit_GR_s.c_str());
    font.Load(FontDescription(Layout::VptScale(unit_height * ratio_dpi)));
    ascent_height = UnitSymbolRenderer::GetAscentHeight(font, unit);
    unit_p = pxpt_pos_infos_next_waypoint.At(
        size_text.width, size_text.height - ascent_height * 1.6);
    canvas.Select(font);
    UnitSymbolRenderer::Draw(canvas, unit_p, unit,
                             look_infobox.unit_fraction_pen);
  }
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
    unsigned task_size, [[maybe_unused]] const NavigatorLook &look) noexcept
{
  const int rc_height = canvas.GetHeight();
  const int rc_width = canvas.GetWidth();

  const WaypointRendererSettings &waypoint_settings =
      CommonInterface::GetMapSettings().waypoint;
  const WaypointLook &waypoint_look = UIGlobals::GetMapLook().waypoint;

  WaypointIconRenderer waypoint_icon_renderer{waypoint_settings, waypoint_look,
                                              canvas};
  const PixelPoint position_waypoint_left{rc_width * 7 / 200,
                                          rc_height * 1 / 2};
  // const PixelPoint position_waypoint_centered{rc_width*42/200,
  // rc_height*1/2};
  const PixelPoint position_waypoint_right{
      rc_width * 98 / 100 - rc_height * 15 / 100, rc_height * 38 / 100};

  // CALCULATE REACHABILITY
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
