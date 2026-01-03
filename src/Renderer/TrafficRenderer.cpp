// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/TrafficLook.hpp"
#include "FLARM/Traffic.hpp"
#include "GliderLink/Traffic.hpp"
#include "Traffic/UnifiedTraffic.hpp"
#include "Math/Screen.hpp"
#include "util/Macros.hpp"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

[[gnu::pure]]
static const Pen &
GetTrafficSourcePen(const TrafficLook &traffic_look,
                    UnifiedTraffic::Source source) noexcept
{
  switch (source) {
  case UnifiedTraffic::Source::FLARM_DIRECT:
    return traffic_look.source_pen_flarm;
  case UnifiedTraffic::Source::GLIDERLINK:
    return traffic_look.source_pen_gliderlink;
  case UnifiedTraffic::Source::OGN_CLOUD:
    return traffic_look.source_pen_ogn;
  case UnifiedTraffic::Source::SKYLINES:
    return traffic_look.source_pen_skylines;
  case UnifiedTraffic::Source::STRATUX:
    return traffic_look.source_pen_stratux;
  }
  return traffic_look.source_pen_flarm;  // Default to FLARM
}

void
TrafficRenderer::Draw(Canvas &canvas, const TrafficLook &traffic_look,
                      bool fading,
                      const FlarmTraffic &traffic, const Angle angle,
                      const FlarmColor color, const PixelPoint pt,
                      UnifiedTraffic::Source source) noexcept
{
  // Create point array that will form that arrow polygon
  // Target: 48x48 pixels on a 100 DPI screen
  // At 100 DPI: 1 point = 100/72 ≈ 1.39 pixels
  // So 48 pixels = 48 * 72 / 100 ≈ 34.56 points ≈ 35 points
  // We'll use 24 points as base (which scales to ~33 pixels at 100 DPI)
  // and scale up to get ~48 pixels
  
  // Original arrow shape: { -4, 6 }, { 0, -8 }, { 4, 6 }, { 0, 3 }
  // Height: from -8 to +6 = 14 units, Width: from -4 to +4 = 8 units
  // Scale to 3x larger: { -12, 18 }, { 0, -24 }, { 12, 18 }, { 0, 9 }
  // New height: from -24 to +18 = 42 units, Width: from -12 to +12 = 24 units
  
  // Scale the original arrow coordinates by 3x
  BulkPixelPoint arrow[] = {
    { -12, 18 },
    { 0, -24 },
    { 12, 18 },
    { 0, 9 },
  };

  // Calculate scale factor for PolygonRotateShift
  // PolygonRotateShift scales coordinates: if scale=100, coordinates -50 to +50 become -100 to +100 pixels
  // Our arrow height is 42 units (from -24 to +18)
  // We want final height of ~48 pixels at 100 DPI
  // At 100 DPI: 48 pixels = 48 * 72 / 100 ≈ 34.56 points
  // Scale factor needed: 48 pixels / 42 units * 100 ≈ 114
  // But we want DPI-aware scaling, so use Layout::PtScale to get actual pixel size
  // Target: 48 pixels equivalent = Layout::PtScale(48 * 72 / 100) ≈ Layout::PtScale(35)
  const unsigned target_points = 35;  // ~48 pixels at 100 DPI
  const unsigned target_px = Layout::PtScale(target_points);
  // Arrow height in coordinate units is 42, so scale factor = target_px * 100 / 42
  const int scale_factor = (int)(target_px * 100 / 42);
  const int final_scale = scale_factor > 0 ? scale_factor : 114;

  // Rotate and shift the arrow to the right position and angle
  // Use calculated scale factor for DPI-based sizing
  PolygonRotateShift(arrow, pt, angle, final_scale);

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

  // Draw traffic source outline around the arrow (not a circle)
  if (!fading) {
    canvas.Select(GetTrafficSourcePen(traffic_look, source));
    canvas.SelectHollowBrush();
    // Draw outline around the arrow polygon
    canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));
  }

  // Draw team color circle (inner circle, if team color is set)
  switch (color) {
  case FlarmColor::GREEN:
    canvas.Select(traffic_look.team_pen_green);
    canvas.SelectHollowBrush();
    canvas.DrawCircle(pt, Layout::FastScale(11u));
    break;
  case FlarmColor::BLUE:
    canvas.Select(traffic_look.team_pen_blue);
    canvas.SelectHollowBrush();
    canvas.DrawCircle(pt, Layout::FastScale(11u));
    break;
  case FlarmColor::YELLOW:
    canvas.Select(traffic_look.team_pen_yellow);
    canvas.SelectHollowBrush();
    canvas.DrawCircle(pt, Layout::FastScale(11u));
    break;
  case FlarmColor::MAGENTA:
    canvas.Select(traffic_look.team_pen_magenta);
    canvas.SelectHollowBrush();
    canvas.DrawCircle(pt, Layout::FastScale(11u));
    break;
  default:
    break;
  }
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
