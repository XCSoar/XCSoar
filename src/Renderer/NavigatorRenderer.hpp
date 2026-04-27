// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Task/TaskType.hpp"
#include "Engine/Waypoint/Ptr.hpp"
#include "Look/InfoBoxLook.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "Units/Unit.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/dim/Rect.hpp"
#include "util/StaticString.hxx"

#include <cstdint>

struct NavigatorLook;
struct TaskLook;
struct TaskSummary;
class Waypoint;

enum class NavType : std::uint8_t {
  NAVIGATOR_LITE_ONE_LINE,
  NAVIGATOR_LITE_TWO_LINES,
  NAVIGATOR,
  NAVIGATOR_DETAILED,
};

class NavigatorRenderer {
  /* ---- layout / Update() cache ------------------------------------ */
  unsigned int canvas_width{};
  unsigned int canvas_height{};

  const MoreData *basic{};
  const DerivedInfo *calculated{};
  bool has_started{};

  const RoughTimeDelta utc_no_offset{};
  RoughTimeDelta utc_offset{};

  /* ---- generated text buffers ------------------------------------ */
  StaticString<20> waypoint_distance_s;
  StaticString<20> waypoint_altitude_s;
  StaticString<20> waypoint_GR_s;
  Angle bearing_diff{};
  StaticString<20> waypoint_direction_s;
  StaticString<100> infos_waypoint_s;
  StaticString<60> infos_waypoint_units_dist_alt_s;
  StaticString<60> infos_waypoint_units_dist_alt_GR_s;

  StaticString<20> current_speed_s;
  StaticString<20> current_altitude_s;
  StaticString<20> waypoint_average_speed_s;

  StaticString<50> waypoint_name_s;

  StaticString<8> time_elapsed_s;
  StaticString<8> time_start_s;
  StaticString<8> time_local_s;
  StaticString<8> time_planned_s;
  StaticString<8> arrival_planned_s;
  StaticString<20> times_local_elapsed_s;
  StaticString<20> times_arrival_planned_s;

  /* ---- draw state (fonts, positions, clip rects) -------------------- */
  Font font;
  unsigned int font_height{};
  PixelPoint pxpt_pos_infos_waypoint{};
  unsigned int text_size_infos_waypoint{};
  PixelSize size_text{};
  unsigned int unit_height{};
  unsigned int ascent_height{};
  PixelPoint pp_pos_unit{};
  Unit unit{Unit::KILOMETER};

  int pos_x_speed_altitude{};

  unsigned pos_x_waypoint_name{};
  unsigned pos_y_waypoint_name{};
  unsigned pos_x_end_waypoint_name{};
  unsigned sz_waypoint_name{};

  void GenerateStringsWaypointInfos(NavType nav_type, TaskType tp) noexcept;

  void GenerateStringsCurrentFlightInfo(TaskType tp) noexcept;

  void GenerateStringWaypointName(const Waypoint &wp_current) noexcept;

  void GenerateStringsTimesInfo(TaskType tp) noexcept;

  void SetTextColor(Canvas &canvas, const NavigatorLook &look_nav) noexcept;

  /**
   * After #unit is set, draw the unit symbol beside the value row at
   * #pxpt_pos_infos_waypoint using #text_for_width to measure the value span.
   */
  void DrawWaypointInfoValueUnit(Canvas &canvas, const InfoBoxLook &look_infobox,
                                 const char *text_for_width) noexcept;

  void DrawWaypointInfos(Canvas &canvas, NavType nav_type,
                         const InfoBoxLook &look_infobox) noexcept;

  void DrawCurrentFlightInfos(Canvas &canvas, NavType nav_type,
                              const InfoBoxLook &look_infobox) noexcept;

  void DrawWaypointName(Canvas &canvas, NavType nav_type) noexcept;

  void DrawTimesInfo(Canvas &canvas, const PixelRect &rc,
                     NavType nav_type) noexcept;

  void DrawAverageSpeed(Canvas &canvas, NavType nav_type,
                        const InfoBoxLook &look_infobox) noexcept;

  void DrawDirectionArrowNorthAnnulus(Canvas &canvas, NavType nav_type,
                                      const TaskLook &look_task) noexcept;

public:
  void Update(const Canvas &canvas) noexcept;

  void DrawFrame(Canvas &canvas, const PixelRect &rc,
                 const NavigatorLook &look_nav) noexcept;

  void DrawTaskTextsArrow(Canvas &canvas, TaskType tp,
                          const Waypoint &wp_current, const PixelRect &rc,
                          NavType nav_type,
                          [[maybe_unused]] const bool is_nav_top_position,
                          const NavigatorLook &look_nav,
                          const TaskLook &look_task,
                          const InfoBoxLook &look_infobox) noexcept;

  void DrawProgressTask(const TaskSummary &summary, Canvas &canvas,
                        const PixelRect &rc, const NavigatorLook &look_nav,
                        const TaskLook &look_task) noexcept;

  void DrawWaypointsIconsTitle(Canvas &canvas, WaypointPtr waypoint_before,
                               WaypointPtr waypoint_current, unsigned task_size,
                               const NavigatorLook &look_nav,
                               NavType nav_type) noexcept;
};
