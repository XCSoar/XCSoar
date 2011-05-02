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

#ifndef CHART_HPP
#define CHART_HPP

#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Screen/Color.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Point.hpp"
#include "Compiler.h"

#include <tchar.h>
#include <vector>

class LeastSquares;
class Canvas;
class Brush;

class Chart
{
private:
  Canvas &canvas;
  PixelRect rc;

  Pen pens[5];

public:
  int PaddingLeft;
  int PaddingBottom;

public:
  Chart(Canvas &the_canvas, const PixelRect the_rc);

  enum Style {
    STYLE_BLUETHIN,
    STYLE_REDTHICK,
    STYLE_DASHGREEN,
    STYLE_MEDIUMBLACK,
    STYLE_THINDASHPAPER
  };

  void Reset();

  void DrawBarChart(const LeastSquares &lsdata);
  void DrawFilledLineGraph(const LeastSquares &lsdata);
  void DrawLineGraph(const LeastSquares &lsdata, Pen &pen);
  void DrawLineGraph(const LeastSquares &lsdata, enum Style Style);
  void DrawTrend(const LeastSquares &lsdata, enum Style Style);
  void DrawTrendN(const LeastSquares &lsdata, enum Style Style);
  void DrawLine(const fixed xmin, const fixed ymin,
                const fixed xmax, const fixed ymax, Pen &pen);
  void DrawLine(const fixed xmin, const fixed ymin,
                const fixed xmax, const fixed ymax, enum Style Style);
  void DrawFilledLine(const fixed xmin, const fixed ymin,
                const fixed xmax, const fixed ymax, Color &colour);
  void DrawFilledY(const std::vector< std::pair<fixed, fixed> > &vals, const Brush &brush);
  void DrawDot(const fixed x, const fixed y, const int width);

  void ScaleYFromData(const LeastSquares &lsdata);
  void ScaleXFromData(const LeastSquares &lsdata);
  void ScaleYFromValue(const fixed val);
  void ScaleXFromValue(const fixed val);
  void ScaleMakeSquare();

  void StyleLine(const RasterPoint l1, const RasterPoint l2, Pen &pen);
  void StyleLine(const RasterPoint l1, const RasterPoint l2, enum Style Style);

  void ResetScale();

  static void FormatTicText(TCHAR *text, const fixed val, const fixed step);
  void DrawXGrid(fixed tic_step, const fixed zero, Pen &pen,
                 fixed unit_step, bool draw_units = false);
  void DrawXGrid(const fixed tic_step, const fixed zero, enum Style Style,
                 const fixed unit_step, bool draw_units = false);
  void DrawYGrid(fixed tic_step, const fixed zero, Pen &pen,
                 fixed unit_step, bool draw_units = false);
  void DrawYGrid(const fixed tic_step, const fixed zero, enum Style Style,
                 const fixed unit_step, bool draw_units = false);

  void DrawXLabel(const TCHAR *text);
  void DrawYLabel(const TCHAR *text);
  void DrawLabel(const TCHAR *text, const fixed xv, const fixed yv);
  void DrawArrow(const fixed x, const fixed y, const fixed mag,
                 const Angle angle, enum Style Style);
  void DrawNoData();

  fixed getYmin() const { return y_min; }
  fixed getYmax() const { return y_max; }
  fixed getXmin() const { return x_min; }
  fixed getXmax() const { return x_max; }

  gcc_pure
  long screenX(fixed x) const;

  gcc_pure
  long screenY(fixed y) const;

  gcc_pure
  long screenS(fixed s) const;

  Canvas& get_canvas() { return canvas; }

private:
  fixed yscale;
  fixed xscale;
  fixed y_min, x_min;
  fixed x_max, y_max;
  bool unscaled_x;
  bool unscaled_y;
};

#endif
