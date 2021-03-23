/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "util/ReusableArray.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "Look/ChartLook.hpp"
#include "util/StringBuffer.hxx"

#include <tchar.h>

template<typename T> struct ConstBuffer;
struct DoublePoint2D;
class XYDataStore;
class LeastSquares;
class Canvas;
class Brush;
class Pen;

/**
 * Render a chart.
 *
 * How to use: construct, SetXLabel()/SetYLabel(), Begin(), Draw*(),
 * Finish().
 */
class ChartRenderer
{
  const ChartLook &look;

  Canvas &canvas;
  PixelRect rc;
  PixelRect rc_chart;

  BasicStringBuffer<TCHAR, 64> x_label, y_label;

  ReusableArray<BulkPixelPoint> point_buffer;

  struct Axis {
    double scale, min, max;
    bool unscaled = true;

    [[gnu::pure]]
    int ToScreen(double value) const noexcept;
  } x, y;

  int x_label_left, y_label_bottom;

  const int minor_tick_size;

public:
  enum UnitFormat {
    NONE,
    NUMERIC,
    TIME
  };

public:
  ChartRenderer(const ChartLook &look, Canvas &the_canvas,
                const PixelRect &the_rc,
                const bool has_padding=true) noexcept;

  void SetXLabel(const TCHAR *text) noexcept;
  void SetXLabel(const TCHAR *text, const TCHAR *unit) noexcept;

  void SetYLabel(const TCHAR *text) noexcept;
  void SetYLabel(const TCHAR *text, const TCHAR *unit) noexcept;

  /**
   * Prepare for drawing; this method calculates the layout.  Call
   * this after all setup methods have been called (e.g. SetXLabel()),
   * and before drawing starts.
   */
  void Begin() noexcept;

  /**
   * Finish drawing.  Call this after drawing is finished.
   */
  void Finish() noexcept;

  const PixelRect &GetChartRect() const noexcept {
    return rc_chart;
  }

  void DrawBarChart(const XYDataStore &lsdata) noexcept;

  void DrawFilledLineGraph(ConstBuffer<DoublePoint2D> src, bool swap=false) noexcept;
  void DrawLineGraph(ConstBuffer<DoublePoint2D> src,
                     const Pen &pen, bool swap=false) noexcept;
  void DrawLineGraph(ConstBuffer<DoublePoint2D> src,
                     ChartLook::Style style, bool swap=false) noexcept;

  void DrawFilledLineGraph(const XYDataStore &lsdata, bool swap=false) noexcept;
  void DrawLineGraph(const XYDataStore &lsdata, const Pen &pen, bool swap=false) noexcept;
  void DrawLineGraph(const XYDataStore &lsdata, ChartLook::Style style, bool swap=false) noexcept;
  void DrawTrend(const LeastSquares &lsdata, ChartLook::Style style) noexcept;
  void DrawTrendN(const LeastSquares &lsdata, ChartLook::Style style) noexcept;
  void DrawLine(double xmin, double ymin,
                double xmax, double ymax, const Pen &pen) noexcept;
  void DrawLine(double xmin, double ymin,
                double xmax, double ymax, ChartLook::Style style) noexcept;
  void DrawFilledLine(double xmin, double ymin,
                      double xmax, double ymax,
                      const Brush &brush) noexcept;
  void DrawFilledY(ConstBuffer<DoublePoint2D> vals,
                   const Brush &brush,
                   const Pen *pen=nullptr) noexcept;
  void DrawDot(double x, double y, const unsigned width) noexcept;
  void DrawImpulseGraph(const XYDataStore &lsdata, const Pen &pen) noexcept;
  void DrawImpulseGraph(const XYDataStore &lsdata, ChartLook::Style style) noexcept;
  void DrawWeightBarGraph(const XYDataStore &lsdata) noexcept;

  void ScaleYFromData(const LeastSquares &lsdata) noexcept;
  void ScaleXFromData(const LeastSquares &lsdata) noexcept;
  void ScaleYFromValue(double val) noexcept;
  void ScaleXFromValue(double val) noexcept;

  [[gnu::pure]]
  static BasicStringBuffer<TCHAR, 32> FormatTicText(double val, double step,
                                                    UnitFormat units) noexcept;

  void DrawXGrid(double tic_step, double unit_step,
                 UnitFormat units = UnitFormat::NONE) noexcept;
  void DrawYGrid(double tic_step, double unit_step,
                 UnitFormat units = UnitFormat::NONE) noexcept;

  void DrawLabel(const TCHAR *text, double xv, double yv) noexcept;
  void DrawNoData(const TCHAR *text) noexcept;
  void DrawNoData() noexcept;

  void DrawBlankRectangle(double x_min, double y_min,
                          double x_max, double y_max) noexcept;

  double GetYMin() const noexcept { return y.min; }
  double GetYMax() const noexcept { return y.max; }
  double GetXMin() const noexcept { return x.min; }
  double GetXMax() const noexcept { return x.max; }

  [[gnu::pure]]
  int ScreenX(double x) const noexcept;

  [[gnu::pure]]
  int ScreenY(double y) const noexcept;

  [[gnu::pure]]
  PixelPoint ToScreen(double x, double y) const noexcept {
    return PixelPoint{ ScreenX(x), ScreenY(y) };
  }

  Canvas &GetCanvas() noexcept { return canvas; }

  const ChartLook &GetLook() const noexcept {
    return look;
  }
};

#endif
