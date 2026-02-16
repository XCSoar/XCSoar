// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/ReusableArray.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "Look/ChartLook.hpp"
#include "util/StringBuffer.hxx"

#include <span>

#include <tchar.h>

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

  BasicStringBuffer<char, 64> x_label, y_label;

  ReusableArray<BulkPixelPoint> point_buffer;

  struct Axis {
    double scale, min, max;
    bool unscaled = true;

    constexpr int ToScreen(double value) const noexcept {
      return int((value - min) * scale);
    }
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

  void SetXLabel(const char *text) noexcept;
  void SetXLabel(const char *text, const char *unit) noexcept;

  void SetYLabel(const char *text) noexcept;
  void SetYLabel(const char *text, const char *unit) noexcept;

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

  void DrawFilledLineGraph(std::span<const DoublePoint2D> src, bool swap=false) noexcept;
  void DrawLineGraph(std::span<const DoublePoint2D> src,
                     const Pen &pen, bool swap=false) noexcept;
  void DrawLineGraph(std::span<const DoublePoint2D> src,
                     ChartLook::Style style, bool swap=false) noexcept;

  void DrawFilledLineGraph(const XYDataStore &lsdata, bool swap=false) noexcept;
  void DrawLineGraph(const XYDataStore &lsdata, const Pen &pen, bool swap=false) noexcept;
  void DrawLineGraph(const XYDataStore &lsdata, ChartLook::Style style, bool swap=false) noexcept;
  void DrawTrend(const LeastSquares &lsdata, ChartLook::Style style) noexcept;
  void DrawTrendN(const LeastSquares &lsdata, ChartLook::Style style) noexcept;
  void DrawLine(DoublePoint2D min, DoublePoint2D max,
                const Pen &pen) noexcept;
  void DrawLine(DoublePoint2D min, DoublePoint2D max,
                ChartLook::Style style) noexcept;
  void DrawFilledLine(DoublePoint2D min, DoublePoint2D max,
                      const Brush &brush) noexcept;
  void DrawFilledY(std::span<const DoublePoint2D> vals,
                   const Brush &brush,
                   const Pen *pen=nullptr) noexcept;
  void DrawDot(DoublePoint2D p, const unsigned width) noexcept;
  void DrawImpulseGraph(const XYDataStore &lsdata, const Pen &pen) noexcept;
  void DrawImpulseGraph(const XYDataStore &lsdata, ChartLook::Style style) noexcept;
  void DrawWeightBarGraph(const XYDataStore &lsdata) noexcept;

  void ScaleYFromData(const LeastSquares &lsdata) noexcept;
  void ScaleXFromData(const LeastSquares &lsdata) noexcept;
  void ScaleYFromValue(double val) noexcept;
  void ScaleXFromValue(double val) noexcept;

  [[gnu::pure]]
  static BasicStringBuffer<char, 32> FormatTicText(double val, double step,
                                                    UnitFormat units) noexcept;

  void DrawXGrid(double tic_step, double unit_step,
                 UnitFormat units = UnitFormat::NONE) noexcept;
  void DrawYGrid(double tic_step, double unit_step,
                 UnitFormat units = UnitFormat::NONE) noexcept;

  void DrawLabel(DoublePoint2D v, const char *text) noexcept;
  void DrawNoData(const char *text) noexcept;
  void DrawNoData() noexcept;

  void DrawBlankRectangle(DoublePoint2D min, DoublePoint2D max) noexcept;

  double GetYMin() const noexcept { return y.min; }
  double GetYMax() const noexcept { return y.max; }
  double GetXMin() const noexcept { return x.min; }
  double GetXMax() const noexcept { return x.max; }

  [[gnu::pure]]
  int ScreenX(double _x) const noexcept {
    return rc_chart.left + x.ToScreen(_x);
  }

  [[gnu::pure]]
  int ScreenY(double _y) const noexcept {
    return rc_chart.bottom - y.ToScreen(_y);
  }

  [[gnu::pure]]
  PixelPoint ToScreen(DoublePoint2D p) const noexcept {
    return {ScreenX(p.x), ScreenY(p.y)};
  }

  Canvas &GetCanvas() noexcept { return canvas; }

  const ChartLook &GetLook() const noexcept {
    return look;
  }
};
