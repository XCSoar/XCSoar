/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Screen/Point.hpp"
#include "Screen/BulkPoint.hpp"
#include "Look/ChartLook.hpp"
#include "Language/Language.hpp"
#include "Compiler.h"

#include <tchar.h>
#include <vector>

class XYDataStore;
class LeastSquares;
class Canvas;
class Brush;
class Pen;

class ChartRenderer
{
  const ChartLook &look;

  Canvas &canvas;
  PixelRect rc;
  PixelRect rc_chart;
  int minor_tick_size;

  ReusableArray<BulkPixelPoint> point_buffer;

  struct Axis {
    double scale, min, max;
    bool unscaled;

    void Reset();

    int ToScreen(double value) const;
  } x, y;

  void SetPadding(bool do_pad);

public:
  int padding_text;
  const PixelRect GetChartRect() const {
    return rc_chart;
  }

  enum UnitFormat {
    NONE,
    NUMERIC,
    TIME
  };

public:
  ChartRenderer(const ChartLook &look, Canvas &the_canvas,
                const PixelRect the_rc,
                const bool has_padding=true);

  void DrawBarChart(const XYDataStore &lsdata);
  void DrawFilledLineGraph(const XYDataStore &lsdata, bool swap=false);
  void DrawLineGraph(const XYDataStore &lsdata, const Pen &pen, bool swap=false);
  void DrawLineGraph(const XYDataStore &lsdata, ChartLook::Style style, bool swap=false);
  void DrawTrend(const LeastSquares &lsdata, ChartLook::Style style);
  void DrawTrendN(const LeastSquares &lsdata, ChartLook::Style style);
  void DrawLine(double xmin, double ymin,
                double xmax, double ymax, const Pen &pen);
  void DrawLine(double xmin, double ymin,
                double xmax, double ymax, ChartLook::Style style);
  void DrawFilledLine(double xmin, double ymin,
                      double xmax, double ymax,
                      const Brush &brush);
  void DrawFilledY(const std::vector<std::pair<double, double>> &vals, const Brush &brush,
                   const Pen *pen=nullptr);
  void DrawDot(double x, double y, const unsigned width);
  void DrawImpulseGraph(const XYDataStore &lsdata, const Pen &pen);
  void DrawImpulseGraph(const XYDataStore &lsdata, ChartLook::Style style);
  void DrawWeightBarGraph(const XYDataStore &lsdata);

  void ScaleYFromData(const LeastSquares &lsdata);
  void ScaleXFromData(const LeastSquares &lsdata);
  void ScaleYFromValue(double val);
  void ScaleXFromValue(double val);

  void ResetScale();

  static void FormatTicText(TCHAR *text, double val, double step, UnitFormat units);

  void DrawXGrid(double tic_step, double unit_step, UnitFormat units = UnitFormat::NONE);
  void DrawYGrid(double tic_step, double unit_step, UnitFormat units = UnitFormat::NONE);

  void DrawXLabel(const TCHAR *text);
  void DrawXLabel(const TCHAR *text, const TCHAR *unit);

  void DrawYLabel(const TCHAR *text);
  void DrawYLabel(const TCHAR *text, const TCHAR *unit);

  void DrawLabel(const TCHAR *text, double xv, double yv);
  void DrawNoData(const TCHAR *text = _("No data"));

  void DrawBlankRectangle(double x_min, double y_min,
                          double x_max, double y_max);

  double GetYMin() const { return y.min; }
  double GetYMax() const { return y.max; }
  double GetXMin() const { return x.min; }
  double GetXMax() const { return x.max; }

  gcc_pure
  int ScreenX(double x) const;

  gcc_pure
  int ScreenY(double y) const;

  gcc_pure
  PixelPoint ToScreen(double x, double y) const {
    return PixelPoint{ ScreenX(x), ScreenY(y) };
  }

  Canvas& GetCanvas() { return canvas; }

  const ChartLook &GetLook() const {
    return look;
  }
};

#endif
