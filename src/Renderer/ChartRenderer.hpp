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

#ifndef XCSOAR_CHART_RENDERER_HPP
#define XCSOAR_CHART_RENDERER_HPP

#include "Util/ReusableArray.hpp"
#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Screen/Point.hpp"
#include "Look/ChartLook.hpp"
#include "Compiler.h"

#include <tchar.h>
#include <vector>

class LeastSquares;
class Canvas;
class Brush;
class Pen;

class ChartRenderer
{
  const ChartLook &look;

  Canvas &canvas;
  PixelRect rc;

  ReusableArray<RasterPoint> point_buffer;

  struct Axis {
    fixed scale, min, max;
    bool unscaled;

    void Reset();
  } x, y;

public:
  int padding_left;
  int padding_bottom;

public:
  ChartRenderer(const ChartLook &look, Canvas &the_canvas,
                const PixelRect the_rc);

  void DrawBarChart(const LeastSquares &lsdata);
  void DrawFilledLineGraph(const LeastSquares &lsdata);
  void DrawLineGraph(const LeastSquares &lsdata, const Pen &pen);
  void DrawLineGraph(const LeastSquares &lsdata, ChartLook::Style style);
  void DrawTrend(const LeastSquares &lsdata, ChartLook::Style style);
  void DrawTrendN(const LeastSquares &lsdata, ChartLook::Style style);
  void DrawLine(const fixed xmin, const fixed ymin,
                const fixed xmax, const fixed ymax, const Pen &pen);
  void DrawLine(const fixed xmin, const fixed ymin,
                const fixed xmax, const fixed ymax, ChartLook::Style style);
  void DrawFilledLine(const fixed xmin, const fixed ymin,
                      const fixed xmax, const fixed ymax,
                      const Brush &brush);
  void DrawFilledY(const std::vector< std::pair<fixed, fixed> > &vals, const Brush &brush,
                   const Pen* pen= NULL);
  void DrawDot(const fixed x, const fixed y, const PixelScalar width);

  void ScaleYFromData(const LeastSquares &lsdata);
  void ScaleXFromData(const LeastSquares &lsdata);
  void ScaleYFromValue(const fixed val);
  void ScaleXFromValue(const fixed val);

  void ResetScale();

  static void FormatTicText(TCHAR *text, const fixed val, const fixed step);
  void DrawXGrid(fixed tic_step, const Pen &pen,
                 fixed unit_step, bool draw_units = false);
  void DrawXGrid(const fixed tic_step, ChartLook::Style style,
                 const fixed unit_step, bool draw_units = false);
  void DrawYGrid(fixed tic_step, const Pen &pen,
                 fixed unit_step, bool draw_units = false);
  void DrawYGrid(const fixed tic_step, ChartLook::Style style,
                 const fixed unit_step, bool draw_units = false);

  void DrawXLabel(const TCHAR *text);
  void DrawXLabel(const TCHAR *text, const TCHAR *unit);

  void DrawYLabel(const TCHAR *text);
  void DrawYLabel(const TCHAR *text, const TCHAR *unit);

  void DrawLabel(const TCHAR *text, const fixed xv, const fixed yv);
  void DrawNoData();

  fixed GetYMin() const { return y.min; }
  fixed GetYMax() const { return y.max; }
  fixed GetXMin() const { return x.min; }
  fixed GetXMax() const { return x.max; }

  gcc_pure
  PixelScalar ScreenX(fixed x) const;

  gcc_pure
  PixelScalar ScreenY(fixed y) const;

  gcc_pure
  RasterPoint ToScreen(fixed x, fixed y) const {
    return RasterPoint{ ScreenX(x), ScreenY(y) };
  }

  Canvas& GetCanvas() { return canvas; }

  const ChartLook &GetLook() const {
    return look;
  }
};

#endif
