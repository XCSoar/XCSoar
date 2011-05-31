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

#ifndef TRACE_HISTORY_RENDERER_HPP
#define TRACE_HISTORY_RENDERER_HPP

#include "Screen/Point.hpp"
#include "Math/fixed.hpp"

struct TraceHistoryLook;
class Chart;
class Canvas;
class TraceVariableHistory;

class TraceHistoryRenderer {
  const TraceHistoryLook &look;

public:
  TraceHistoryRenderer(const TraceHistoryLook &_look)
    :look(_look) {}

  void RenderVario(Canvas& canvas,
                   const PixelRect rc,
                   const TraceVariableHistory& var,
                   const bool centered = false,
                   const fixed mc=fixed_zero) const;

private:
  void scale_chart(Chart &chart,
                   const TraceVariableHistory& var,
                   const bool centered) const;

  void render_axis(Chart &chart,
                   const TraceVariableHistory& var) const;

  void render_line(Chart &chart,
                   const TraceVariableHistory& var) const;

  void render_filled_posneg(Chart &chart,
                            const TraceVariableHistory& var) const;
};

#endif
