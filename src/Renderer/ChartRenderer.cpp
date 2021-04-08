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

#include "ChartRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Math/LeastSquares.hpp"
#include "Math/Point2D.hpp"
#include "util/StaticString.hxx"
#include "util/StringFormat.hpp"
#include "util/TruncateString.hpp"
#include "util/ConstBuffer.hxx"

#include <cassert>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

int
ChartRenderer::Axis::ToScreen(double value) const noexcept
{
  return int((value - min) * scale);
}

ChartRenderer::ChartRenderer(const ChartLook &_look, Canvas &the_canvas,
                             const PixelRect &the_rc,
                             const bool has_padding) noexcept
  :look(_look), canvas(the_canvas),
   rc(the_rc),
   minor_tick_size(Layout::VptScale(4))
{
  x_label.clear();
  y_label.clear();
}

void
ChartRenderer::SetXLabel(const TCHAR *text) noexcept
{
  CopyTruncateString(x_label.data(), x_label.capacity(), text);
}

void
ChartRenderer::SetXLabel(const TCHAR *text, const TCHAR *unit) noexcept
{
  StringFormat(x_label.data(), x_label.capacity(),
               _T("%s [%s]"), text, unit);
}

void
ChartRenderer::SetYLabel(const TCHAR *text, const TCHAR *unit) noexcept
{
  StringFormat(y_label.data(), y_label.capacity(),
               _T("%s [%s]"), text, unit);
}

void
ChartRenderer::SetYLabel(const TCHAR *text) noexcept
{
  CopyTruncateString(y_label.data(), y_label.capacity(), text);
}

void
ChartRenderer::Begin() noexcept
{
  rc_chart = rc;

  if (!x_label.empty()) {
    /* make room for X axis labels below the chart */
    const auto size = look.axis_label_font.TextSize(x_label.c_str());

    rc_chart.bottom -= size.height + Layout::GetTextPadding() * 2;
    x_label_left = rc.right - size.width - Layout::GetTextPadding() * 2;
  }

  if (!y_label.empty()) {
    /* make room for Y axis labels left of the chart */
    const auto size = look.axis_label_font.TextSize(y_label.c_str());

    rc_chart.left += std::max(size.width + Layout::GetTextPadding() * 2,
                              Layout::VptScale(30));
    y_label_bottom = rc.top - size.height - Layout::GetTextPadding() * 2;
  }

  if (!x_label.empty() || !y_label.empty())
    canvas.DrawFilledRectangle(rc_chart, COLOR_WHITE);
}

void
ChartRenderer::Finish() noexcept
{
  if (!x_label.empty()) {
    /* draw the X axis label */

    canvas.Select(look.axis_label_font);
    canvas.SetBackgroundTransparent();

    PixelSize tsize = canvas.CalcTextSize(x_label.c_str());
    int x = rc.right - tsize.width - Layout::GetTextPadding();
    int y = rc.bottom - tsize.height - Layout::GetTextPadding();

    canvas.DrawText({x, y}, x_label.c_str());
  }

  if (!y_label.empty()) {
    /* draw the Y axis label */

    canvas.Select(look.axis_label_font);
    canvas.SetBackgroundTransparent();

    canvas.DrawText(rc.WithPadding(Layout::GetTextPadding()).GetTopLeft(),
                    y_label.c_str());
  }
}

void
ChartRenderer::ScaleYFromData(const LeastSquares &lsdata) noexcept
{
  if (lsdata.IsEmpty())
    return;

  if (y.unscaled) {
    y.min = lsdata.GetMinY();
    y.max = lsdata.GetMaxY();
    y.unscaled = false;
  } else {
    y.min = std::min(y.min, lsdata.GetMinY());
    y.max = std::max(y.max, lsdata.GetMaxY());
  }

  if (lsdata.HasResult()) {
    auto y0 = lsdata.GetYAtMinX();
    auto y1 = lsdata.GetYAtMaxX();
    y.min = std::min({y.min, y0, y1});
    y.max = std::max({y.max, y0, y1});
  }

  if (fabs(y.max - y.min) > 50) {
    y.scale = (y.max - y.min);
    if (y.scale > 0)
      y.scale = rc_chart.GetHeight() / y.scale;
  } else {
    y.scale = 2000;
  }
}

void
ChartRenderer::ScaleXFromData(const LeastSquares &lsdata) noexcept
{
  if (lsdata.IsEmpty())
    return;

  if (x.unscaled) {
    x.min = lsdata.GetMinX();
    x.max = lsdata.GetMaxX();
    x.unscaled = false;
  } else {
    x.min = std::min(x.min, lsdata.GetMinX());
    x.max = std::max(x.max, lsdata.GetMaxX());
  }

  x.scale = (x.max - x.min);
  if (x.scale > 0)
    x.scale = rc_chart.GetWidth() / x.scale;
}

void
ChartRenderer::ScaleYFromValue(const double value) noexcept
{
  if (y.unscaled) {
    y.min = value;
    y.max = value;
    y.unscaled = false;
  } else {
    y.min = std::min(value, y.min);
    y.max = std::max(value, y.max);
  }

  y.scale = (y.max - y.min);
  if (y.scale > 0)
    y.scale = rc_chart.GetHeight() / y.scale;
}

void
ChartRenderer::ScaleXFromValue(const double value) noexcept
{
  if (x.unscaled) {
    x.min = value;
    x.max = value;
    x.unscaled = false;
  } else {
    x.min = std::min(value, x.min);
    x.max = std::max(value, x.max);
  }

  x.scale = (x.max - x.min);
  if (x.scale > 0)
    x.scale = rc_chart.GetWidth() / x.scale;
}

void
ChartRenderer::DrawLabel(const TCHAR *text,
                         const double xv, const double yv) noexcept
{
  canvas.Select(look.label_font);
  canvas.SetBackgroundTransparent();

  auto tsize = canvas.CalcTextSize(text);
  auto pt = ToScreen(xv, yv);
  canvas.SelectNullPen();
  {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.Select(look.label_blank_brush);

    const PixelSize rect_size = tsize + PixelSize{Layout::GetTextPadding() * 2};
    canvas.DrawRectangle(PixelRect::Centered(pt, rect_size));
  }
  canvas.DrawText(pt - tsize / 2u, text);
}

void
ChartRenderer::DrawNoData(const TCHAR *text) noexcept
{
  canvas.Select(look.label_font);
  canvas.SetBackgroundTransparent();

  canvas.DrawText(rc.CenteredTopLeft(canvas.CalcTextSize(text)), text);
}

void
ChartRenderer::DrawNoData() noexcept
{
  DrawNoData(_("No data"));
}

void
ChartRenderer::DrawTrend(const LeastSquares &lsdata,
                         ChartLook::Style style) noexcept
{
  if (!lsdata.HasResult())
    return;

  if (x.unscaled || y.unscaled)
    return;

  auto xmin = x.min;
  auto xmax = x.max;
  auto ymin = lsdata.GetYAt(x.min);
  auto ymax = lsdata.GetYAt(x.max);

  DrawLine(xmin, ymin, xmax, ymax, look.GetPen(style));
}

void
ChartRenderer::DrawTrendN(const LeastSquares &lsdata,
                          ChartLook::Style style) noexcept
{
  if (!lsdata.HasResult())
    return;

  if (x.unscaled || y.unscaled)
    return;

  double xmin = 0.5;
  double xmax = lsdata.GetCount() + 0.5;
  double ymin = lsdata.GetYAtMinX();
  double ymax = lsdata.GetYAtMaxX();

  DrawLine(xmin, ymin, xmax, ymax, look.GetPen(style));
}

void
ChartRenderer::DrawLine(const double xmin, const double ymin,
                        const double xmax, const double ymax,
                        const Pen &pen) noexcept
{
  if (x.unscaled || y.unscaled)
    return;

  assert(pen.IsDefined());
  canvas.Select(pen);
  canvas.DrawLine(ToScreen(xmin, ymin), ToScreen(xmax, ymax));
}

void 
ChartRenderer::DrawFilledLine(const double xmin, const double ymin,
                              const double xmax, const double ymax,
                              const Brush &brush) noexcept
{
  BulkPixelPoint line[4];

  line[0] = ToScreen(xmin, ymin);
  line[1] = ToScreen(xmax, ymax);

  line[2].x = line[1].x;
  line[2].y = ScreenY(0);
  line[3].x = line[0].x;
  line[3].y = line[2].y;

  canvas.Select(brush);
  canvas.SelectNullPen();
  canvas.DrawTriangleFan(line, 4);
}

void
ChartRenderer::DrawLine(const double xmin, const double ymin,
                        const double xmax, const double ymax,
                        ChartLook::Style style) noexcept
{
  DrawLine(xmin, ymin, xmax, ymax, look.GetPen(style));
}

void
ChartRenderer::DrawBarChart(const XYDataStore &lsdata) noexcept
{
  if (x.unscaled || y.unscaled)
    return;

  canvas.Select(look.bar_brush);
  canvas.SelectNullPen();

  double xmin = rc_chart.left + 1.2 * x.scale;
  double xmax = rc_chart.left + 1.8 * x.scale;
  const int ymin = ScreenY(y.min);

  for (const auto &i : lsdata.GetSlots()) {
    int ymax = ScreenY(i.y);

    canvas.DrawRectangle({int(xmin), ymin, int(xmax), ymax});

    xmin += x.scale;
    xmax += x.scale;
  }
}

template<typename T>
static BulkPixelPoint *
PrepareLineGraph(BulkPixelPoint *p, ConstBuffer<T> src,
                 const ChartRenderer &chart, bool swap) noexcept
{
  if (swap) {
    for (const auto &i : src)
      *p++ = chart.ToScreen(i.y, i.x);
  } else {
    for (const auto &i : src)
      *p++ = chart.ToScreen(i.x, i.y);
  }

  return p;
}

template<typename T>
static BulkPixelPoint *
PrepareFilledLineGraph(BulkPixelPoint *p, ConstBuffer<T> src,
                       const ChartRenderer &chart, bool swap) noexcept
{
  const auto &p0 = *p;

  p = PrepareLineGraph(p, src, chart, swap);

  const auto &last = p[-1];
  const auto &rc_chart = chart.GetChartRect();
  if (swap) {
    *p++ = BulkPixelPoint(rc_chart.left, last.y);
    *p++ = BulkPixelPoint(rc_chart.left, p0.y);
  } else {
    *p++ = BulkPixelPoint(last.x, rc_chart.bottom);
    *p++ = BulkPixelPoint(p0.x, rc_chart.bottom);
  }

  return p;
}

void
ChartRenderer::DrawFilledLineGraph(ConstBuffer<DoublePoint2D> src,
                                   bool swap) noexcept
{
  const unsigned n = src.size + 2;
  auto *points = point_buffer.get(n);

  [[maybe_unused]] auto *p =
    PrepareFilledLineGraph(points, src, *this, swap);
  assert(p == points + n);

  canvas.DrawPolygon(points, n);
}

void
ChartRenderer::DrawLineGraph(ConstBuffer<DoublePoint2D> src,
                             const Pen &pen, bool swap) noexcept
{
  assert(src.size >= 2);

  const unsigned n = src.size;
  auto *points = point_buffer.get(n);

  [[maybe_unused]] auto *p =
    PrepareLineGraph(points, src, *this, swap);
  assert(p == points + n);

  canvas.Select(pen);
  canvas.DrawPolyline(points, n);
}

void
ChartRenderer::DrawLineGraph(ConstBuffer<DoublePoint2D> src,
                             ChartLook::Style style, bool swap) noexcept
{
  DrawLineGraph(src, look.GetPen(style), swap);
}

void
ChartRenderer::DrawFilledLineGraph(const XYDataStore &lsdata,
                                   bool swap) noexcept
{
  const auto slots = lsdata.GetSlots();
  assert(slots.size >= 2);

  const unsigned n = slots.size + 2;
  auto *points = point_buffer.get(n);

  [[maybe_unused]] auto *p =
    PrepareFilledLineGraph(points, slots, *this, swap);
  assert(p == points + n);

  canvas.DrawPolygon(points, n);
}

void
ChartRenderer::DrawLineGraph(const XYDataStore &lsdata, const Pen &pen,
                             bool swap) noexcept
{
  const auto slots = lsdata.GetSlots();
  assert(slots.size >= 2);

  const unsigned n = slots.size;
  auto *points = point_buffer.get(n);

  [[maybe_unused]] auto *p =
    PrepareLineGraph(points, slots, *this, swap);
  assert(p == points + n);

  canvas.Select(pen);
  canvas.DrawPolyline(points, n);
}

void
ChartRenderer::DrawLineGraph(const XYDataStore &lsdata,
                             ChartLook::Style style, bool swap) noexcept
{
  DrawLineGraph(lsdata, look.GetPen(style), swap);
}

BasicStringBuffer<TCHAR, 32>
ChartRenderer::FormatTicText(const double val, const double step,
                             UnitFormat units) noexcept
{
  BasicStringBuffer<TCHAR, 32> buffer;

  if (units == UnitFormat::TIME) {
    int hh = (int)(val);
    int mm = (int)((val-hh)*60);
    StringFormat(buffer.data(), buffer.capacity(), _T("%02d:%02d"), hh, mm);
  } else {
    if (step < 1) {
      StringFormat(buffer.data(), buffer.capacity(), _T("%.1f"), val);
    } else {
      StringFormat(buffer.data(), buffer.capacity(), _T("%.0f"), val);
    }
  }

  return buffer;
}

void
ChartRenderer::DrawXGrid(double tic_step, double unit_step,
                         UnitFormat unit_format) noexcept
{
  assert(tic_step > 0);

  canvas.Select(look.axis_value_font);
  canvas.SetBackgroundTransparent();

  PixelPoint line[4];

  /** the minimum next position of the text, to avoid overlapping */
  int next_text = rc.left;

  /* increase tic step so graph not too crowded */
  while ((x.max - x.min) / tic_step > 10) {
    tic_step *= 2;
    unit_step *= 2;
  }

  line[0].y = line[2].y = rc_chart.top;
  line[1].y = line[3].y = rc_chart.bottom;
  line[2].y += minor_tick_size;
  line[3].y -= minor_tick_size;

  const int y = line[1].y + Layout::GetTextPadding();

  auto start = (int)(x.min / tic_step) * tic_step;

  for (auto xval = start; xval <= x.max; xval += tic_step) {
    int xmin = ScreenX(xval);

    for (auto xmval = xval; xmval < xval+tic_step; xmval+= tic_step/5) {
      const auto xmmin = ScreenX(xmval);
      line[0].x = line[1].x = line[2].x = line[3].x = xmmin;
      if (xmmin >= rc_chart.left && xmmin <= rc.right) {
        canvas.Select(look.GetPen(ChartLook::STYLE_GRIDMINOR));
        canvas.DrawLine(line[0], line[2]);
        canvas.DrawLine(line[1], line[3]);

        if (xmval == xval) {
          if (xval == 0) {
            canvas.Select(look.GetPen(ChartLook::STYLE_GRIDZERO));
          } else {
            canvas.Select(look.GetPen(ChartLook::STYLE_GRID));
          }
          canvas.DrawLine(line[0], line[1]);

          if (unit_format != UnitFormat::NONE) {
            const auto unit_text = FormatTicText(xval * unit_step / tic_step,
                                                 unit_step, unit_format);
            const auto w = canvas.CalcTextSize(unit_text.c_str()).width;
            xmin -= w/2;
            if ((xmin >= next_text) && ((int)(xmin + Layout::VptScale(30)) < x_label_left)) {
              canvas.DrawText({xmin, y}, unit_text.c_str());
              next_text = xmin + w + Layout::GetTextPadding();
            }
          }
        }
      }
    }
  }
}

void
ChartRenderer::DrawYGrid(double tic_step, double unit_step,
                         UnitFormat unit_format) noexcept
{
  assert(tic_step > 0);

  canvas.Select(look.axis_value_font);
  canvas.SetBackgroundTransparent();

  PixelPoint line[4];

  /* increase tic step so graph not too crowded */
  while ((y.max-y.min)/tic_step > 10) {
    tic_step *= 2;
    unit_step *= 2;
  }

  line[0].x = line[2].x = rc_chart.left;
  line[1].x = line[3].x = rc_chart.right;
  line[2].x += minor_tick_size;
  line[3].x -= minor_tick_size;

  const int x = line[0].x - Layout::GetTextPadding();

  auto start = (int)(y.min / tic_step) * tic_step;

  for (auto yval = start; yval <= y.max; yval += tic_step) {
    const int ymin = ScreenY(yval);

    for (auto ymval = yval; ymval < yval+tic_step; ymval+= tic_step/5) {
      const auto ymmin = ScreenY(ymval);
      line[0].y = line[1].y = line[2].y = line[3].y = ymmin;
      if (ymmin >= rc_chart.top && ymmin <= rc.bottom) {
        canvas.Select(look.GetPen(ChartLook::STYLE_GRIDMINOR));
        canvas.DrawLine(line[0], line[2]);
        canvas.DrawLine(line[1], line[3]);

        if (ymval == yval) {
          if (yval == 0) {
            canvas.Select(look.GetPen(ChartLook::STYLE_GRIDZERO));
          } else {
            canvas.Select(look.GetPen(ChartLook::STYLE_GRID));
          }
          canvas.DrawLine(line[0], line[1]);

          if ((unit_format != UnitFormat::NONE) && (ymin > (int)(y_label_bottom + Layout::VptScale(30)))) {
            const auto unit_text = FormatTicText(yval * unit_step / tic_step,
                                                 unit_step, unit_format);
            const auto c = canvas.CalcTextSize(unit_text.c_str());
            canvas.DrawText({std::max(x - (int)c.width, rc.left + (int)Layout::GetTextPadding()), ymin - (int)c.height / 2},
                            unit_text.c_str());
          }
        }
      }
    }
  }
}

int
ChartRenderer::ScreenX(double _x) const noexcept
{
  return rc_chart.left + x.ToScreen(_x);
}

int
ChartRenderer::ScreenY(double _y) const noexcept
{
  return rc_chart.bottom - y.ToScreen(_y);
}

void
ChartRenderer::DrawFilledY(ConstBuffer<DoublePoint2D> vals,
                           const Brush &brush, const Pen *pen) noexcept
{
  if (vals.size < 2)
    return;
  const unsigned fsize = vals.size + 2;
  auto *line = point_buffer.get(fsize);

  PrepareLineGraph(line + 2, vals, *this, false);

  line[0].x = rc_chart.left;
  line[0].y = line[fsize-1].y;
  line[1].x = rc_chart.left;
  line[1].y = line[2].y;

  canvas.Select(brush);
  if (pen == nullptr) {
    canvas.SelectNullPen();
  } else {
    canvas.Select(*pen);
  }
  canvas.DrawPolygon(line, fsize);
}

void
ChartRenderer::DrawDot(const double x, const double y,
                       const unsigned _width) noexcept
{
  auto p = ToScreen(x, y);

  const int width = _width;
  const BulkPixelPoint line[4] = {
    { p.x, p.y - width },
    { p.x - width, p.y },
    { p.x, p.y + width },
    { p.x + width, p.y },
  };
  canvas.SelectNullPen();
  canvas.DrawTriangleFan(line, 4);
}

void
ChartRenderer::DrawBlankRectangle(double x_min, double y_min,
                                  double x_max, double y_max) noexcept
{
  if (x.unscaled || y.unscaled)
    return;
  canvas.Select(look.blank_brush);
  canvas.DrawRectangle({ScreenX(x_min), ScreenY(y_min), ScreenX(x_max), ScreenY(y_max)});
}

void
ChartRenderer::DrawImpulseGraph(const XYDataStore &lsdata,
                                const Pen &pen) noexcept
{
  const auto slots = lsdata.GetSlots();
  assert(slots.size >= 1);

  canvas.Select(pen);
  for (const auto &i : slots) {
    auto pt_base = ToScreen(i.x, y.min);
    auto pt_top = ToScreen(i.x, i.y);
    canvas.DrawLine(pt_base, pt_top);
  }
}

void
ChartRenderer::DrawImpulseGraph(const XYDataStore &lsdata,
                                ChartLook::Style style) noexcept
{
  DrawImpulseGraph(lsdata, look.GetPen(style));
}

void
ChartRenderer::DrawWeightBarGraph(const XYDataStore &lsdata) noexcept
{
  const auto slots = lsdata.GetSlots();

  canvas.SelectNullPen();

  for (const auto &i : slots) {
    auto pt_base = ToScreen(i.x, y.min);
    auto pt_top = ToScreen(i.x+i.weight, i.y);
    canvas.DrawRectangle({pt_base.x, pt_base.y, pt_top.x, pt_top.y});
  }
}
