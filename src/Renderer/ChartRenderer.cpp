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

#include "ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Math/LeastSquares.hpp"
#include "Util/StaticString.hxx"

#include <assert.h>
#include <stdio.h>
#include <windef.h> /* for MAX_PATH */

void
ChartRenderer::Axis::Reset()
{
  unscaled = true;
  scale = 0;
  min = 0;
  max = 0;
}

int
ChartRenderer::Axis::ToScreen(double value) const
{
  return int((value - min) * scale);
}

void
ChartRenderer::ResetScale()
{
  x.Reset();
  y.Reset();
}

ChartRenderer::ChartRenderer(const ChartLook &_look, Canvas &the_canvas,
                             const PixelRect the_rc)
  :look(_look), canvas(the_canvas), rc(the_rc),
   padding_left(24), padding_bottom(19)
{
  ResetScale();
}

void
ChartRenderer::ScaleYFromData(const LeastSquares &lsdata)
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
      y.scale = (rc.GetHeight() - padding_bottom) / y.scale;
  } else {
    y.scale = 2000;
  }
}

void
ChartRenderer::ScaleXFromData(const LeastSquares &lsdata)
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
    x.scale = (rc.GetWidth() - padding_left) / x.scale;
}

void
ChartRenderer::ScaleYFromValue(const double value)
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
    y.scale = (rc.GetHeight() - padding_bottom) / y.scale;
}

void
ChartRenderer::ScaleXFromValue(const double value)
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
    x.scale = (rc.GetWidth() - padding_left) / x.scale;
}

void
ChartRenderer::DrawLabel(const TCHAR *text, const double xv, const double yv)
{
  canvas.Select(look.label_font);
  canvas.SetBackgroundTransparent();

  auto tsize = canvas.CalcTextSize(text);
  auto pt = ToScreen(xv, yv);
  canvas.DrawText(pt.x - tsize.cx / 2, pt.y - tsize.cy / 2, text);
}

void
ChartRenderer::DrawNoData(const TCHAR *text)
{
  canvas.Select(look.label_font);
  canvas.SetBackgroundTransparent();

  PixelSize tsize = canvas.CalcTextSize(text);

  int x = (rc.left + rc.right - tsize.cx) / 2;
  int y = (rc.top + rc.bottom - tsize.cy) / 2;

  canvas.DrawText(x, y, text);
}

void
ChartRenderer::DrawXLabel(const TCHAR *text)
{
  canvas.Select(look.axis_label_font);
  canvas.SetBackgroundTransparent();

  PixelSize tsize = canvas.CalcTextSize(text);
  int x = rc.right - tsize.cx - Layout::GetTextPadding();
  int y = rc.bottom - tsize.cy - Layout::GetTextPadding();

  canvas.DrawText(x, y, text);
}

void
ChartRenderer::DrawXLabel(const TCHAR *text, const TCHAR *unit)
{
  assert(text != nullptr);
  assert(unit != nullptr);

  StaticString<64> buffer;
  buffer.UnsafeFormat(_T("%s [%s]"), text, unit);
  DrawXLabel(buffer);
}

void
ChartRenderer::DrawYLabel(const TCHAR *text)
{
  canvas.Select(look.axis_label_font);
  canvas.SetBackgroundTransparent();

  int x = rc.left + Layout::GetTextPadding();
  int y = rc.top + Layout::GetTextPadding();

  canvas.DrawText(x, y, text);
}

void
ChartRenderer::DrawYLabel(const TCHAR *text, const TCHAR *unit)
{
  assert(text != nullptr);
  assert(unit != nullptr);

  StaticString<64> buffer;
  buffer.UnsafeFormat(_T("%s [%s]"), text, unit);
  DrawYLabel(buffer);
}

void
ChartRenderer::DrawTrend(const LeastSquares &lsdata, ChartLook::Style style)
{
  if (!lsdata.HasResult())
    return;

  if (x.unscaled || y.unscaled)
    return;

  auto xmin = lsdata.GetMinX();
  auto xmax = lsdata.GetMaxX();
  auto ymin = lsdata.GetYAtMinX();
  auto ymax = lsdata.GetYAtMaxX();

  DrawLine(xmin, ymin, xmax, ymax, look.GetPen(style));
}

void
ChartRenderer::DrawTrendN(const LeastSquares &lsdata, ChartLook::Style style)
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
                        const double xmax, const double ymax, const Pen &pen)
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
                              const Brush &brush)
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
                        ChartLook::Style style)
{
  DrawLine(xmin, ymin, xmax, ymax, look.GetPen(style));
}

void
ChartRenderer::DrawBarChart(const LeastSquares &lsdata)
{
  if (x.unscaled || y.unscaled)
    return;

  canvas.Select(look.bar_brush);
  canvas.SelectNullPen();

  const auto &slots = lsdata.GetSlots();
  for (unsigned i = 0, n = slots.size(); i != n; i++) {
    int xmin((i + 1.2) * x.scale
             + (rc.left + padding_left));
    int ymin = ScreenY(y.min);
    int xmax((i + 1.8) * x.scale
             + (rc.left + padding_left));
    int ymax = ScreenY(slots[i].y);
    canvas.Rectangle(xmin, ymin, xmax, ymax);
  }
}

void
ChartRenderer::DrawFilledLineGraph(const LeastSquares &lsdata)
{
  const auto &slots = lsdata.GetSlots();
  assert(slots.size() >= 2);

  const unsigned n = slots.size() + 2;
  auto *points = point_buffer.get(n);

  auto *p = points;
  for (const auto &i : slots)
    *p++ = ToScreen(i.x, i.y);
  const auto &last = p[-1];
  *p++ = BulkPixelPoint(last.x, rc.bottom - padding_bottom);
  *p++ = BulkPixelPoint(points[0].x, rc.bottom - padding_bottom);

  assert(p == points + n);

  canvas.DrawPolygon(points, n);
}

void
ChartRenderer::DrawLineGraph(const LeastSquares &lsdata, const Pen &pen)
{
  const auto &slots = lsdata.GetSlots();
  assert(slots.size() >= 2);

  const unsigned n = slots.size();
  auto *points = point_buffer.get(n);

  auto *p = points;
  for (const auto &i : slots)
    *p++ = ToScreen(i.x, i.y);
  assert(p == points + n);

  canvas.Select(pen);
  canvas.DrawPolyline(points, n);
}

void
ChartRenderer::DrawLineGraph(const LeastSquares &lsdata,
                             ChartLook::Style style)
{
  DrawLineGraph(lsdata, look.GetPen(style));
}

void
ChartRenderer::FormatTicText(TCHAR *text, const double val, const double step)
{
  if (step < 1) {
    _stprintf(text, _T("%.1f"), (double)val);
  } else {
    _stprintf(text, _T("%.0f"), (double)val);
  }
}

void
ChartRenderer::DrawXGrid(const double tic_step, ChartLook::Style style,
                         const double unit_step, bool draw_units)
{
  DrawXGrid(tic_step, look.GetPen(style), unit_step, draw_units);
}

void
ChartRenderer::DrawXGrid(double tic_step, const Pen &pen,
                         double unit_step, bool draw_units)
{
  assert(tic_step > 0);

  canvas.Select(pen);
  canvas.Select(look.axis_value_font);
  canvas.SetBackgroundTransparent();

  PixelPoint line[2];

  /** the minimum next position of the text, to avoid overlapping */
  int next_text = rc.left;

  /* increase tic step so graph not too crowded */
  while ((x.max - x.min) / tic_step > 10) {
    tic_step *= 2;
    unit_step *= 2;
  }
  //  bool do_units = ((x.max-zero)/tic_step)<10;

  line[0].y = rc.top;
  line[1].y = rc.bottom - padding_bottom;

  const int y = line[1].y - canvas.GetFontHeight();

  auto start = (int)(x.min / tic_step) * tic_step;

  for (auto xval = start; xval <= x.max; xval += tic_step) {
    const int xmin = ScreenX(xval);
    line[0].x = line[1].x = xmin;

    // STYLE_THINDASHPAPER
    if (xmin >= rc.left + padding_left && xmin <= rc.right) {
      canvas.DrawLine(line[0], line[1]);

      if (draw_units && xmin >= next_text) {
        TCHAR unit_text[MAX_PATH];
        FormatTicText(unit_text, xval * unit_step / tic_step, unit_step);

        canvas.DrawText(xmin, y, unit_text);

        next_text = xmin + canvas.CalcTextSize(unit_text).cx
          + Layout::GetTextPadding();
      }
    }
  }
}

void
ChartRenderer::DrawYGrid(const double tic_step, ChartLook::Style style,
                         const double unit_step, bool draw_units)
{
  DrawYGrid(tic_step, look.GetPen(style), unit_step, draw_units);
}

void
ChartRenderer::DrawYGrid(double tic_step, const Pen &pen,
                         double unit_step, bool draw_units)
{
  assert(tic_step > 0);

  canvas.Select(pen);
  canvas.Select(look.axis_value_font);
  canvas.SetBackgroundTransparent();

  PixelPoint line[2];

  /* increase tic step so graph not too crowded */
  while ((y.max-y.min)/tic_step > 10) {
    tic_step *= 2;
    unit_step *= 2;
  }

  line[0].x = rc.left + padding_left;
  line[1].x = rc.right;

  const int x = line[0].x;

  auto start = (int)(y.min / tic_step) * tic_step;

  for (auto yval = start; yval <= y.max; yval += tic_step) {
    const int ymin = ScreenY(yval);
    line[0].y = line[1].y = ymin;

    // STYLE_THINDASHPAPER
    if (ymin >= rc.top && ymin <= rc.bottom - padding_bottom) {
      canvas.DrawLine(line[0], line[1]);

      if (draw_units) {
        TCHAR unit_text[MAX_PATH];
        FormatTicText(unit_text, yval * unit_step / tic_step, unit_step);

        canvas.DrawText(x, ymin, unit_text);
      }
    }
  }
}

int
ChartRenderer::ScreenX(double _x) const
{
  return rc.left + padding_left + x.ToScreen(_x);
}

int
ChartRenderer::ScreenY(double _y) const
{
  return rc.bottom - padding_bottom - y.ToScreen(_y);
}

void
ChartRenderer::DrawFilledY(const std::vector<std::pair<double, double>> &vals,
                           const Brush &brush, const Pen* pen)
{
  if (vals.size()<2)
    return;
  const unsigned fsize = vals.size()+2;
  auto *line = point_buffer.get(fsize);

  for (unsigned i = 0; i < vals.size(); ++i)
    line[i + 2] = ToScreen(vals[i].first, vals[i].second);

  line[0].x = rc.left + padding_left;
  line[0].y = line[fsize-1].y;
  line[1].x = rc.left + padding_left;
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
ChartRenderer::DrawDot(const double x, const double y, const unsigned _width)
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
