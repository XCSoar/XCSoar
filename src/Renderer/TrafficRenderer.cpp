// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/TrafficLook.hpp"
#include "FLARM/Traffic.hpp"
#include "GliderLink/Traffic.hpp"
#include "Math/Screen.hpp"
#include "util/Macros.hpp"
#include "Asset.hpp"

#include <algorithm>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

static void
DrawFlarmArrow(Canvas &canvas, const TrafficLook &traffic_look,
               bool fading, const FlarmTraffic &traffic,
               const Angle angle, const FlarmColor color,
               const PixelPoint pt,
               int arrow_scale, unsigned circle_radius) noexcept
{
  BulkPixelPoint arrow[] = {
    { -4, 6 },
    { 0, -8 },
    { 4, 6 },
    { 0, 3 },
  };

  PolygonRotateShift(arrow, pt, angle, arrow_scale);

  if (fading) {
    canvas.Select(traffic_look.fading_pen);

#ifdef ENABLE_OPENGL
    canvas.Select(traffic_look.fading_brush);
#else
    /* we have no alpha blending - don't fill the shape */
    canvas.SelectHollowBrush();
#endif

#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));
  } else {
    switch (traffic.alarm_level) {
    case FlarmTraffic::AlarmType::LOW:
    case FlarmTraffic::AlarmType::INFO_ALERT:
      canvas.Select(traffic_look.warning_brush);
      break;
    case FlarmTraffic::AlarmType::IMPORTANT:
    case FlarmTraffic::AlarmType::URGENT:
      canvas.Select(traffic_look.alarm_brush);
      break;
    case FlarmTraffic::AlarmType::NONE:
      if (traffic.relative_altitude > (const RoughAltitude)50) {
        canvas.Select(traffic_look.safe_above_brush);
      } else if (traffic.relative_altitude > (const RoughAltitude)-50) {
        canvas.Select(traffic_look.warning_in_altitude_range_brush);
      } else {
        canvas.Select(traffic_look.safe_below_brush);
      }
      break;
    }

    canvas.SelectBlackPen();
    canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));
  }

  switch (color) {
  case FlarmColor::GREEN:
    canvas.Select(traffic_look.team_pen_green);
    break;
  case FlarmColor::BLUE:
    canvas.Select(traffic_look.team_pen_blue);
    break;
  case FlarmColor::YELLOW:
    canvas.Select(traffic_look.team_pen_yellow);
    break;
  case FlarmColor::MAGENTA:
    canvas.Select(traffic_look.team_pen_magenta);
    break;
  default:
    return;
  }

  canvas.SelectHollowBrush();
  canvas.DrawCircle(pt, circle_radius);
}

void
TrafficRenderer::Draw(Canvas &canvas, const TrafficLook &traffic_look,
                      bool fading,
                      const FlarmTraffic &traffic, const Angle angle,
                      const FlarmColor color, const PixelPoint pt) noexcept
{
  DrawFlarmArrow(canvas, traffic_look, fading, traffic, angle, color, pt,
                 Layout::Scale(100U), Layout::FastScale(11u));
}

void
TrafficRenderer::DrawList(Canvas &canvas, const TrafficLook &traffic_look,
                          const FlarmTraffic &traffic, const Angle angle,
                          const FlarmColor color, const PixelPoint pt,
                          unsigned icon_size) noexcept
{
  /* The arrow template spans 14 units vertically (-8 to +6). */
  constexpr unsigned arrow_span = 14;
  const int arrow_scale =
    std::max(int(icon_size) * 50 / int(arrow_span), 1);
  const unsigned circle_radius =
    std::max(icon_size / 3, Layout::ScalePenWidth(1));

  DrawFlarmArrow(canvas, traffic_look, false, traffic, angle, color, pt,
                 arrow_scale, circle_radius);
}

void
TrafficRenderer::Draw(Canvas &canvas, const TrafficLook &traffic_look,
                      [[maybe_unused]] const GliderLinkTraffic &traffic,
                      const Angle angle, const PixelPoint pt) noexcept
{
  BulkPixelPoint arrow[] = {
    { -4, 6 },
    { 0, -8 },
    { 4, 6 },
    { 0, 3 },
  };

  canvas.Select(traffic_look.safe_above_brush);

  if (IsDithered())
    canvas.Select(Pen(Layout::FastScale(2), COLOR_BLACK));
  else
    canvas.SelectBlackPen();

  PolygonRotateShift(arrow, pt, angle, Layout::Scale(100U));
  canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));

  canvas.SelectHollowBrush();
  canvas.DrawCircle(pt, Layout::FastScale(11u));
}
