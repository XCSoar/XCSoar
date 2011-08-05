/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Navigation/TraceHistory.hpp"
#include "Screen/Chart.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/TraceHistoryLook.hpp"
#include "Look/VarioLook.hpp"

#include <algorithm>

void
TraceHistoryRenderer::scale_chart(Chart &chart,
                                  const TraceVariableHistory& var,
                                  const bool centered) const
{
  chart.PaddingBottom = 0;
  chart.PaddingLeft = 0;
  chart.ScaleXFromValue(fixed(0));
  chart.ScaleXFromValue(fixed(var.capacity()-1));

  fixed vmin = fixed_zero;
  fixed vmax = fixed_zero;
  for (TraceVariableHistory::const_iterator it = var.begin();
       it != var.end(); ++it) {
    vmin = std::min(*it, vmin);
    vmax = std::max(*it, vmax);
  }
  if (!(vmax>vmin)) {
    vmax += fixed_one;
  }
  if (centered) {
    vmax = std::max(vmax, -vmin);
    vmin = std::min(vmin, -vmax);
  }
  chart.ScaleYFromValue(vmax);
  chart.ScaleYFromValue(vmin);
}

void
TraceHistoryRenderer::render_axis(Chart &chart,
                                  const TraceVariableHistory& var) const
{
  chart.DrawLine(fixed_zero, fixed_zero, 
                 fixed(var.capacity()-1), fixed_zero, 
                 look.axis_pen);
}


void 
TraceHistoryRenderer::render_line(Chart &chart,
                                  const TraceVariableHistory& var) const
{
  fixed x_last, y_last;
  unsigned i=0;
  for (TraceVariableHistory::const_iterator it = var.begin();
       it != var.end(); ++it, ++i) {
    fixed x= fixed(i);
    fixed y= *it;
    if (i)
      chart.DrawLine(x_last, y_last, x, y, look.line_pen);
    x_last = x;
    y_last = y;
  }
}

static int sgn(const fixed x) {
  if (positive(x))
    return 1;
  if (negative(x))
    return -1;
  return 0;
}

void 
TraceHistoryRenderer::render_filled_posneg(Chart &chart,
                                           const TraceVariableHistory& var) const
{
  fixed x_last(fixed_zero), y_last(fixed_zero);
  unsigned i=0;
  for (TraceVariableHistory::const_iterator it = var.begin();
       it != var.end(); ++it, ++i) {
    fixed x= fixed(i);
    fixed y= *it;
    if (i) {
      if (sgn(y)*sgn(y_last)<0) {
        if (positive(y_last))
          chart.DrawFilledLine(x_last, y_last, x_last+fixed_half, fixed_zero,
                               vario_look.lift_brush);
        else if (negative(y_last))
          chart.DrawFilledLine(x_last, y_last, x_last+fixed_half, fixed_zero,
                               vario_look.sink_brush);
        
        x_last = x-fixed_half;
        y_last = fixed_zero;

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
    chart.get_canvas().white_brush();
  else
    chart.get_canvas().black_brush();
  chart.DrawDot(x_last, y_last, IBLSCALE(2));
}

void
TraceHistoryRenderer::RenderVario(Canvas& canvas,
                                  const PixelRect rc,
                                  const TraceVariableHistory& var,
                                  const bool centered,
                                  const fixed mc) const
{
  Chart chart(chart_look, canvas, rc);
  scale_chart(chart, var, centered);
  chart.ScaleYFromValue(mc);
  // render_line(chart, var);

  if (positive(mc)) {
    canvas.background_transparent();
    chart.DrawLine(fixed_zero, mc, 
                   fixed(var.capacity()-1), mc, 
                   ChartLook::STYLE_DASHGREEN);
  }

  render_filled_posneg(chart, var);
  render_axis(chart, var);
}
