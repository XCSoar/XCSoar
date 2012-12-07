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

#ifndef TRACE_HISTORY_RENDERER_HPP
#define TRACE_HISTORY_RENDERER_HPP

#include "Math/fixed.hpp"

struct PixelRect;
struct TraceHistoryLook;
struct VarioLook;
struct ChartLook;
class ChartRenderer;
class Canvas;
class TraceVariableHistory;

class TraceHistoryRenderer {
  const TraceHistoryLook &look;
  const VarioLook &vario_look;
  const ChartLook &chart_look;

public:
  TraceHistoryRenderer(const TraceHistoryLook &_look,
                       const VarioLook &_vario_look,
                       const ChartLook &_chart_look)
    :look(_look), vario_look(_vario_look), chart_look(_chart_look) {}

  void RenderVario(Canvas& canvas,
                   const PixelRect rc,
                   const TraceVariableHistory& var,
                   const bool centered = false,
                   const fixed mc=fixed(0)) const;

private:
  void scale_chart(ChartRenderer &chart,
                   const TraceVariableHistory& var,
                   const bool centered) const;

  void render_axis(ChartRenderer &chart,
                   const TraceVariableHistory& var) const;

  void render_line(ChartRenderer &chart,
                   const TraceVariableHistory& var) const;

  void render_filled_posneg(ChartRenderer &chart,
                            const TraceVariableHistory& var) const;
};

#endif
