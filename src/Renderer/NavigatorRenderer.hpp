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

  /**
   * Generate the frame(s) of the Navigator and the waypoint.
   */
  void GenerateFrame(const PixelRect &rc, const bool is_frame_main) noexcept;

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
   * Draw the progress of the current task with presntation of each taskpoint
   */
  void DrawProgressTask(const TaskSummary &summary, Canvas &canvas,
                        const PixelRect &rc, const NavigatorLook &look_nav,
                        const TaskLook &look_task) noexcept;
};