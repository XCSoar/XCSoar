// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Ptr.hpp"
#include "Engine/Task/TaskType.hpp"
#include "Look/InfoBoxLook.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "Units/Unit.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/canvas/Canvas.hpp"

#include <array>

struct PixelPoint;
struct BulkPixelPoint;
struct NavigatorLook;
struct InfoBoxLook;
struct AttitudeState;
class Canvas;
class TextRenderer;
class WaypointIconRenderer;
struct TaskLook;
struct TaskSummary;

enum class navType : uint8_t {
    NAVIGATOR_LITE_ONE_LINE,
    NAVIGATOR_LITE_TWO_LINES,
    NAVIGATOR,
    NAVIGATOR_DETAILED
  };

class NavigatorRenderer {
  bool hasCanvasSizeChanged{};
  unsigned int canvas_width{};
  unsigned int canvas_height{};

  // GenerateFrame() members ----------------------------------------
  std::array<BulkPixelPoint, 10> polygone_frame_main;
  std::array<BulkPixelPoint, 10> polygone_frame_detailed;

  const MoreData *basic{};
  const DerivedInfo *calculated{};
  bool has_started{};

  // GenerateStringsWaypointInfos() members -------------------------
  StaticString<20> waypoint_distance_s; // e_WP_Distance
  StaticString<20> waypoint_altitude_s; // WP_AltReq WP_AltDiff or WP_AltArrival
  StaticString<20> waypoint_GR_s; // e_WP_GR glide ratio
  Angle bearing_diff{};
  StaticString<20> waypoint_direction_s; // e_WP_BearingDiff
  StaticString<100> infos_waypoint_s; // dist + alt + GR
  StaticString<60> infos_waypoint_units_dist_alt_s;
  StaticString<60> infos_waypoint_units_dist_alt_GR_s;

  // GenerateStringsCurrentFlightInfo() members ---------------------
  StaticString<20> current_speed_s; // e_Speed_GPS
  StaticString<20> current_altitude_s; // e_HeightGPS
  StaticString<20> waypoint_average_speed_s; // e_SpeedTaskAvg

  // GenerateStringWaypointName() members ---------------------------
  StaticString<50> waypoint_name_s; // e_WP_Name

  // DrawWaypointInfos() members ------------------------------------
  Font font;
  unsigned int font_height{};
  double ratio_dpi{};
  PixelPoint pxpt_pos_infos_waypoint{}; // also used in Waypoint_name /
                                        // Arrow placement
  unsigned int text_size_infos_waypoint{};
  PixelSize size_text{};
  unsigned int unit_height{};
  unsigned int ascent_height{};
  PixelPoint pp_pos_unit{};
  Unit unit{Unit::KILOMETER};
  PixelPoint pp_drawed_text_origin{};
  PixelSize ps_drawed_text_size{};
  PixelRect pr_drawed_text_rect{};

  // DrawCurrentFlightInfos() members -------------------------------
  int pos_x_speed_altitude{}; // also used in DrawWaypointName() /
                              // Arrow placement

  // DrawWaypointName() members -------------------------------------
  unsigned pos_x_waypoint_name{};
  unsigned pos_y_waypoint_name{};
  unsigned pos_x_end_waypoint_name{}; // also used Arrow placement
  unsigned sz_waypoint_name{}; // also used Arrow placement

  ///////////////////////////////////////////////////////////////////
  // generate text --------------------------------------------------
  
  /**
   * Generate the frame(s) of the Navigator and the waypoint.
   */
  void GenerateFrame(const PixelRect &rc, const bool is_frame_main) noexcept;

  /**
   * Generate texts:
   * distance, altitude, (glide ratio, bearing)
   */
  void GenerateStringsWaypointInfos(const enum navType nav_type,
                                 const TaskType tp) noexcept;

  /**
   * Generate texts:
   * current speed, altitude and task average speed
   */
  void GenerateStringsCurrentFlightInfo(const TaskType tp) noexcept;

  /**
   * Generate text: waypoint's name
   */
  void GenerateStringWaypointName(const Waypoint &wp_current) noexcept;

  ///////////////////////////////////////////////////////////////////
  // draw -----------------------------------------------------------
  /**
   * Set the text colour of all strings in the navigator widget 
   */
  void SetTextColor(Canvas &canvas, const NavigatorLook &look_nav) noexcept;

  /**
   * Draw infos_waypoint_s: distance, altitude, (glide ratio, bearing)
   */
  void DrawWaypointInfos(Canvas &canvas, const enum navType nav_type,
                         const InfoBoxLook &look_infobox) noexcept;

  /**
   * Draw current speed / current Altitude
   */
  void DrawCurrentFlightInfos(Canvas &canvas, const enum navType nav_type,
                              const InfoBoxLook &look_infobox) noexcept;

  void DrawWaypointName(Canvas &canvas, const enum navType nav_type) noexcept;
public:
  /**
   * Update all data for generating frame, text and
   */
  void Update(const Canvas &canvas) noexcept;

  /**
   * This function is used to draw the frame of the Navigator and
   * also the frame of the waypoint
   */
  void DrawFrame(Canvas &canvas, const PixelRect &rc,
                 const NavigatorLook &look_nav, const bool is_frame_main) noexcept;

  /**
   * Draw information texts about the current task (ordered task) or
   * the current target (unordered task) + flight infos
   * e.g. waypoint distance, start time, planned duration, current speed...
   */
  void DrawTaskTextsArrow(Canvas &canvas, TaskType tp,
                          const Waypoint &wp_current, const PixelRect &rc,
                          const enum navType nav_type,
                          [[maybe_unused]] const bool isNavTopPosition,
                          const NavigatorLook &look_nav,
                          const TaskLook &look_task,
                          const InfoBoxLook &look_infobox) noexcept;

  /**
   * Draw the progress of the current task with presntation of each taskpoint
   */
  void DrawProgressTask(const TaskSummary &summary, Canvas &canvas,
                        const PixelRect &rc, const NavigatorLook &look_nav,
                        const TaskLook &look_task) noexcept;

  /**
   * Draw the icon of the current task and of the previous task
   */
  void DrawWaypointsIconsTitle(Canvas &canvas,
                               const WaypointPtr waypoint_before,
                               const WaypointPtr waypoint_current,
                               unsigned task_size,
                               const NavigatorLook &look_nav,
                               const enum navType nav_type) noexcept;
};