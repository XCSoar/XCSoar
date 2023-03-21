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
  chart.DrawLine(0, 0,
                 var.capacity() - 1, 0,
                 look.axis_pen);
}

void
TraceHistoryRenderer::render_filled_posneg(ChartRenderer &chart,
                                           const TraceVariableHistory& var) const
{
  double x_last(0), y_last(0);
  unsigned i=0;
  for (auto it = var.begin(); it != var.end(); ++it, ++i) {
    double x = i;
    double y = *it;
    if (i) {
      if (y * y_last < 0) {
        if (y_last > 0)
          chart.DrawFilledLine(x_last, y_last, x_last + 0.5, 0,
                               vario_look.lift_brush);
        else if (y_last < 0)
          chart.DrawFilledLine(x_last, y_last, x_last + 0.5, 0,
                               vario_look.sink_brush);

        x_last = x - 0.5;
        y_last = 0;

      }
      if (y > 0 || y_last > 0)
        chart.DrawFilledLine(x_last, y_last, x, y, vario_look.lift_brush);
      else if (y < 0 || y_last < 0)
        chart.DrawFilledLine(x_last, y_last, x, y, vario_look.sink_brush);
    }
    x_last = x;
    y_last = y;
  }
  if (look.inverse)
    chart.GetCanvas().SelectWhiteBrush();
  else
    chart.GetCanvas().SelectBlackBrush();
  chart.DrawDot(x_last, y_last, Layout::Scale(2));
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
    chart.DrawLine(0, mc,
                   var.capacity() - 1, mc,
                   ChartLook::STYLE_GREENDASH);
  }

  render_filled_posneg(chart, var);
  RenderAxis(chart, var);

  chart.Finish();
}
