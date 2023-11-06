// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ClimbPercentRenderer.hpp"
#include "RadarRenderer.hpp"
#include "NMEA/CirclingInfo.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Math/Angle.hpp"
#include "Look/ClimbPercentLook.hpp"

#include <algorithm>

void
ClimbPercentRenderer::Draw(const CirclingInfo& stats, Canvas &canvas,
                           const PixelRect &rc, bool inverse)
{
  RadarRenderer radar_renderer{Layout::Scale(3U)};
  radar_renderer.UpdateLayout(rc);
  
  const unsigned radius = radar_renderer.GetRadius();
  const auto center = radar_renderer.GetCenter();

  if (stats.time_cruise + stats.time_circling > FloatDuration{}) {

    canvas.SelectNullPen();

    if (stats.noncircling_climb_percentage > 0) {
      canvas.Select(look.brush_noncircling_climb);
      canvas.DrawSegment(center, radius, -Angle::FullCircle()* stats.noncircling_climb_percentage/100, Angle::Zero());
    }

    if (stats.circling_climb_percentage > 0) {
      canvas.Select(look.brush_circling_climb);
      canvas.DrawSegment(center, radius, Angle::Zero(), Angle::FullCircle()* stats.circling_climb_percentage/100);
    }

    if (stats.circling_percentage > stats.circling_climb_percentage) {
      const Pen pen_b(Layout::ScalePenWidth(1), inverse ? COLOR_BLACK : COLOR_WHITE);
      canvas.Select(pen_b);
      canvas.Select(look.brush_circling_descent);
      canvas.DrawSegment(center, radius, Angle::FullCircle()* stats.circling_climb_percentage/100,
                         Angle::FullCircle()*stats.circling_percentage/100);
    }
  }

  const Pen pen_f(Layout::ScalePenWidth(1), inverse ? COLOR_WHITE : COLOR_BLACK);
  canvas.Select(pen_f);
  canvas.SelectHollowBrush();
  radar_renderer.DrawCircle(canvas, radius);
}
