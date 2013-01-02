/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TraceHistoryRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Navigation/TraceHistory.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/TraceHistoryLook.hpp"
#include "Look/VarioLook.hpp"

#include <algorithm>

void
TraceHistoryRenderer::ScaleChart(ChartRenderer &chart,
                                  const TraceVariableHistory& var,
                                  const bool centered) const
{
  chart.padding_bottom = 0;
  chart.padding_left = 0;
  chart.ScaleXFromValue(fixed(0));
  chart.ScaleXFromValue(fixed(var.capacity()-1));

  fixed vmin = fixed(0);
  fixed vmax = fixed(0);
  for (auto it = var.begin(); it != var.end(); ++it) {
    vmin = std::min(*it, vmin);
    vmax = std::max(*it, vmax);
  }
  if (!(vmax>vmin)) {
    vmax += fixed(1);
  }
  if (centered) {
    vmax = std::max(vmax, -vmin);
    vmin = std::min(vmin, -vmax);
  }
  chart.ScaleYFromValue(vmax);
  chart.ScaleYFromValue(vmin);
}

void
TraceHistoryRenderer::RenderAxis(ChartRenderer &chart,
                                  const TraceVariableHistory& var) const
{
  chart.DrawLine(fixed(0), fixed(0), 
                 fixed(var.capacity()-1), fixed(0), 
                 look.axis_pen);
}

void 
TraceHistoryRenderer::render_filled_posneg(ChartRenderer &chart,
                                           const TraceVariableHistory& var) const
{
  fixed x_last(fixed(0)), y_last(fixed(0));
  unsigned i=0;
  for (auto it = var.begin(); it != var.end(); ++it, ++i) {
    fixed x= fixed(i);
    fixed y= *it;
    if (i) {
      if (sgn(y)*sgn(y_last)<0) {
        if (positive(y_last))
          chart.DrawFilledLine(x_last, y_last, x_last+fixed(0.5), fixed(0),
                               vario_look.lift_brush);
        else if (negative(y_last))
          chart.DrawFilledLine(x_last, y_last, x_last+fixed(0.5), fixed(0),
                               vario_look.sink_brush);

        x_last = x-fixed(0.5);
        y_last = fixed(0);

      }
      if (positive(y) || positive(y_last))
        chart.DrawFilledLine(x_last, y_last, x, y, vario_look.lift_brush);
      else if (negative(y) || negative(y_last))
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
                                  const fixed mc) const
{
  ChartRenderer chart(chart_look, canvas, rc);
  ScaleChart(chart, var, centered);
  chart.ScaleYFromValue(mc);

  if (positive(mc)) {
    canvas.SetBackgroundTransparent();
    chart.DrawLine(fixed(0), mc, 
                   fixed(var.capacity()-1), mc, 
                   ChartLook::STYLE_DASHGREEN);
  }

  render_filled_posneg(chart, var);
  RenderAxis(chart, var);
}
