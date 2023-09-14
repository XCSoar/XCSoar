// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TraceHistoryRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Navigation/TraceHistory.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/TraceHistoryLook.hpp"
#include "Look/VarioLook.hpp"

#include <algorithm>

void
TraceHistoryRenderer::ScaleChart(ChartRenderer &chart,
                                  const TraceVariableHistory& var,
                                  const double max,
                                  const double min,
                                  const bool centered) const
{
  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(var.capacity() - 1);

  double vmin = 0;
  double vmax = 0;
  for (auto it = var.begin(); it != var.end(); ++it) {
    vmin = std::min(*it, vmin);
    vmax = std::max(*it, vmax);
  }
  if (!(vmax>vmin)) {
    vmax += 1;
  }
  if (centered) {
    vmax = std::max(vmax, -vmin);
    vmin = std::min(vmin, -vmax);
  }
  chart.ScaleYFromValue(vmax);
  chart.ScaleYFromValue(vmin);
  if (centered) {
    chart.ScaleYFromValue(std::max(max, -min));
    chart.ScaleYFromValue(std::min(-max, min));
  } else {
    chart.ScaleYFromValue(max);
    chart.ScaleYFromValue(min);
  }
}

void
TraceHistoryRenderer::RenderAxis(ChartRenderer &chart,
                                  const TraceVariableHistory& var) const
{
  chart.DrawLine({0, 0}, {var.capacity() - 1., 0},
                 look.axis_pen);
}

void
TraceHistoryRenderer::render_filled_posneg(ChartRenderer &chart,
                                           const TraceVariableHistory& var) const
{
  DoublePoint2D last{0, 0};
  unsigned i=0;
  for (auto it = var.begin(); it != var.end(); ++it, ++i) {
    const DoublePoint2D p{double(i), *it};
    if (i) {
      if (p.y * last.y < 0) {
        if (last.y > 0)
          chart.DrawFilledLine(last, {last.x + 0.5, 0},
                               vario_look.lift_brush);
        else if (last.y < 0)
          chart.DrawFilledLine(last, {last.x + 0.5, 0},
                               vario_look.sink_brush);

        last = {p.x - 0.5, 0};
      }
      if (p.y > 0 || last.y > 0)
        chart.DrawFilledLine(last, p, vario_look.lift_brush);
      else if (p.y < 0 || last.y < 0)
        chart.DrawFilledLine(last, p, vario_look.sink_brush);
    }
    last = p;
  }
  if (look.inverse)
    chart.GetCanvas().SelectWhiteBrush();
  else
    chart.GetCanvas().SelectBlackBrush();
  chart.DrawDot(last, Layout::Scale(2));
}

void
TraceHistoryRenderer::RenderVario(Canvas& canvas,
                                  const PixelRect rc,
                                  const TraceVariableHistory& var,
                                  const bool centered,
                                  const double mc,
                                  const double max,
                                  const double min) const
{
  ChartRenderer chart(chart_look, canvas, rc, false);
  chart.Begin();

  ScaleChart(chart, var, max, min, centered);
  chart.ScaleYFromValue(mc);

  if (mc > 0) {
    canvas.SetBackgroundTransparent();
    chart.DrawLine({0, mc},
                   {var.capacity() - 1., mc},
                   ChartLook::STYLE_GREENDASH);
  }

  render_filled_posneg(chart, var);
  RenderAxis(chart, var);

  chart.Finish();
}
