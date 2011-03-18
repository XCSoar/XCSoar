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
#include <algorithm>

void
TraceHistoryRenderer::scale_chart(Chart &chart,
                                  const TraceVariableHistory& var) {
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
  chart.ScaleYFromValue(vmax);
  chart.ScaleYFromValue(vmin);
}

void
TraceHistoryRenderer::RenderVario(Canvas& canvas,
                                  const RECT rc,
                                  const TraceVariableHistory& var)
{
  Chart chart(canvas, rc);
  scale_chart(chart, var);

  fixed x_last, y_last;
  unsigned i=0;
  for (TraceVariableHistory::const_iterator it = var.begin();
       it != var.end(); ++it, ++i) {
    fixed x= fixed(i);
    fixed y= *it;
    if (i)
      chart.DrawLine(x_last, y_last, x, y, Chart::STYLE_REDTHICK);
    x_last = x;
    y_last = y;
  }
}
