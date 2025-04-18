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

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

void
TrafficRenderer::Draw(Canvas &canvas, const TrafficLook &traffic_look,
                      bool fading,
                      const FlarmTraffic &traffic, const Angle angle,
                      const FlarmColor color, const PixelPoint pt) noexcept
{
  // Create point array that will form that arrow polygon
  BulkPixelPoint arrow[] = {
    { -4, 6 },
    { 0, -8 },
    { 4, 6 },
    { 0, 3 },
  };

  // Rotate and shift the arrow to the right position and angle
  PolygonRotateShift(arrow, pt, angle, Layout::Scale(100U));

  if (fading) {
    canvas.Select(traffic_look.fading_pen);

#ifdef ENABLE_OPENGL
    canvas.Select(traffic_look.fading_brush);
#else
    /* we have no alpha blending - don't fill the shape */
    canvas.SelectHollowBrush();
#endif

    // Draw the arrow
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));
  } else {
    // Select brush depending on AlarmLevel
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

    // Select black pen
    canvas.SelectBlackPen();

    // Draw the arrow
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
  canvas.DrawCircle(pt, Layout::FastScale(11u));
}



void
TrafficRenderer::Draw(Canvas &canvas, const TrafficLook &traffic_look,
                      [[maybe_unused]] const GliderLinkTraffic &traffic,
                      const Angle angle, const PixelPoint pt) noexcept
{
  // Create point array that will form that arrow polygon
  BulkPixelPoint arrow[] = {
    { -4, 6 },
    { 0, -8 },
    { 4, 6 },
    { 0, 3 },
  };

  canvas.Select(traffic_look.safe_above_brush);

  // Select black pen
  if (IsDithered())
    canvas.Select(Pen(Layout::FastScale(2), COLOR_BLACK));
  else
    canvas.SelectBlackPen();

  // Rotate and shift the arrow to the right position and angle
  PolygonRotateShift(arrow, pt, angle, Layout::Scale(100U));

  // Draw the arrow
  canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));

  canvas.SelectHollowBrush();
  canvas.DrawCircle(pt, Layout::FastScale(11u));
}
