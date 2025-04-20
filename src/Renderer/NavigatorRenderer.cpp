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

