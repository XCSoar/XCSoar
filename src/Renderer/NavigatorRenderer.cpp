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
