// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskProgressRenderer.hpp"
#include "RadarRenderer.hpp"
#include "Look/TaskLook.hpp"
#include "Engine/Task/Stats/TaskSummary.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Math/Angle.hpp"

#include <algorithm>

void 
TaskProgressRenderer::Draw(const TaskSummary& summary, Canvas &canvas,
                           const PixelRect &rc, bool inverse)
{
  RadarRenderer radar_renderer{Layout::Scale(3U)};
  radar_renderer.UpdateLayout(rc);
  const unsigned radius = radar_renderer.GetRadius();

  const Angle sweep = Angle::FullCircle() * 0.97;

  if (summary.p_remaining < 0.99) {
    canvas.Select(look.hbGray);
    canvas.SelectNullPen();
    canvas.DrawSegment(radar_renderer.GetCenter(), radius, Angle::Zero(),
                       sweep * (1 -  summary.p_remaining));
  }

  const Pen pen_f(Layout::ScalePenWidth(1), inverse ? COLOR_WHITE : COLOR_BLACK);
  canvas.Select(pen_f);
  canvas.SelectHollowBrush();
  radar_renderer.DrawCircle(canvas, radius);

  unsigned i = 0;
  for (auto it = summary.pts.begin(); it != summary.pts.end(); ++it, ++i) {
    Angle a = sweep * it->p;
    const PixelPoint p = radar_renderer.At(a, radius);
    int w;
    if (i == summary.active) {
      if (it->achieved)
        canvas.Select(look.hbGreen);
      else
        canvas.Select(look.hbOrange);

      w = Layout::Scale(3);
    } else if (i < summary.active) {
      if (it->achieved)
        canvas.Select(look.hbGreen);
      else
        canvas.Select(look.hbNotReachableTerrain);

      w = Layout::Scale(2);
    } else {
      if (it->achieved)
        canvas.Select(look.hbGreen);
      else
        canvas.Select(look.hbLightGray);

      w = Layout::Scale(1);
    }

    canvas.DrawRectangle(PixelRect{p}.WithMargin(w));
  }
}
