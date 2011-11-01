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

#include "Screen/Chart.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Math/FastMath.h"
#include "Math/FastRotation.hpp"
#include "Asset.hpp"
#include "Math/LeastSquares.hpp"

#include <assert.h>
#include <stdio.h>
#include <windef.h> /* for MAX_PATH */

void
Chart::ResetScale()
{
  unscaled_y = true;
  unscaled_x = true;
  xscale = fixed_zero;
  yscale = fixed_zero;
  x_min = fixed_zero;
  x_max = fixed_zero;
  y_min = fixed_zero;
  y_max = fixed_zero;
}

Chart::Chart(const ChartLook &_look,
             Canvas &the_canvas, const PixelRect the_rc)
  :look(_look), canvas(the_canvas), rc(the_rc),
   PaddingLeft(24), PaddingBottom(19)
{
  ResetScale();
}

void
Chart::ScaleYFromData(const LeastSquares &lsdata)
{
  if (!lsdata.sum_n)
    return;

  if (unscaled_y) {
    y_min = lsdata.y_min;
    y_max = lsdata.y_max;
    unscaled_y = false;
  } else {
    y_min = min(y_min, lsdata.y_min);
    y_max = max(y_max, lsdata.y_max);
  }

  if (lsdata.sum_n > 1) {
    fixed y0, y1;
    y0 = lsdata.x_min * lsdata.m + lsdata.b;
    y1 = lsdata.x_max * lsdata.m + lsdata.b;
    y_min = min(y_min, min(y0, y1));
    y_max = max(y_max, max(y0, y1));
  }

  if (fabs(y_max - y_min) > fixed(50)) {
    yscale = (y_max - y_min);
    if (positive(yscale))
      yscale = fixed(rc.bottom - rc.top - PaddingBottom) / yscale;
  } else {
    yscale = fixed(2000);
  }
}

void
Chart::ScaleXFromData(const LeastSquares &lsdata)
{
  if (!lsdata.sum_n)
    return;

  if (unscaled_x) {
    x_min = lsdata.x_min;
    x_max = lsdata.x_max;
    unscaled_x = false;
  } else {
    x_min = min(x_min, lsdata.x_min);
    x_max = max(x_max, lsdata.x_max);
  }

  xscale = (x_max - x_min);
  if (positive(xscale))
    xscale = fixed(rc.right - rc.left - PaddingLeft) / xscale;
}

void
Chart::ScaleYFromValue(const fixed value)
{
  if (unscaled_y) {
    y_min = value;
    y_max = value;
    unscaled_y = false;
  } else {
    y_min = min(value, y_min);
    y_max = max(value, y_max);
  }

  yscale = (y_max - y_min);
  if (positive(yscale))
    yscale = fixed(rc.bottom - rc.top - PaddingBottom) / yscale;
}

void
Chart::ScaleXFromValue(const fixed value)
{
  if (unscaled_x) {
    x_min = value;
    x_max = value;
    unscaled_x = false;
  } else {
    x_min = min(value, x_min);
    x_max = max(value, x_max);
  }

  xscale = (x_max - x_min);
  if (positive(xscale))
    xscale = fixed(rc.right - rc.left - PaddingLeft) / xscale;
}

void
Chart::StyleLine(const RasterPoint l1, const RasterPoint l2, ChartLook::Style Style)
{
  StyleLine(l1, l2, look.GetPen(Style));
}

void
Chart::StyleLine(const RasterPoint l1, const RasterPoint l2, const Pen &pen)
{
  assert(pen.IsDefined());
  canvas.select(pen);
  canvas.line(l1, l2);
}

void
Chart::DrawLabel(const TCHAR *text, const fixed xv, const fixed yv)
{
  PixelSize tsize = canvas.text_size(text);

  PixelScalar x = PixelScalar((xv - x_min) * xscale) + rc.left - tsize.cx / 2 + PaddingLeft;
  PixelScalar y = PixelScalar((y_max - yv) * yscale) + rc.top - tsize.cy / 2;

  canvas.background_transparent();
  canvas.text(x, y, text);
}

void
Chart::DrawNoData()
{
  const TCHAR *text = _("No data");

  PixelSize tsize = canvas.text_size(text);

  PixelScalar x = (rc.left + rc.right - tsize.cx) / 2;
  PixelScalar y = (rc.top + rc.bottom - tsize.cy) / 2;

  canvas.background_transparent();
  canvas.text(x, y, text);
}

void
Chart::DrawXLabel(const TCHAR *text)
{
  canvas.select(*look.axis_label_font);

  PixelSize tsize = canvas.text_size(text);
  PixelScalar x = rc.right - tsize.cx - Layout::Scale(3);
  PixelScalar y = rc.bottom - tsize.cy;

  canvas.background_transparent();
  canvas.text(x, y, text);
}

void
Chart::DrawYLabel(const TCHAR *text)
{
  canvas.select(*look.axis_label_font);

  PixelSize tsize = canvas.text_size(text);
  PixelScalar x = max(PixelScalar(2), PixelScalar(rc.left - tsize.cx));
  PixelScalar y = rc.top;

  canvas.background_transparent();
  canvas.text(x, y, text);
}

void
Chart::DrawTrend(const LeastSquares &lsdata, ChartLook::Style Style)
{
  if (lsdata.sum_n < 2)
    return;

  if (unscaled_x || unscaled_y)
    return;

  fixed xmin, xmax, ymin, ymax;
  xmin = lsdata.x_min;
  xmax = lsdata.x_max;
  ymin = lsdata.x_min * lsdata.m + lsdata.b;
  ymax = lsdata.x_max * lsdata.m + lsdata.b;

  xmin = (xmin - x_min) * xscale + fixed(rc.left + PaddingLeft);
  xmax = (xmax - x_min) * xscale + fixed(rc.left + PaddingLeft);
  ymin = (y_max - ymin) * yscale + fixed(rc.top);
  ymax = (y_max - ymax) * yscale + fixed(rc.top);

  RasterPoint line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(line[0], line[1], Style);
}

void
Chart::DrawTrendN(const LeastSquares &lsdata, ChartLook::Style Style)
{
  if (lsdata.sum_n < 2)
    return;

  if (unscaled_x || unscaled_y)
    return;

  fixed xmin, xmax, ymin, ymax;
  xmin = fixed_half;
  xmax = fixed(lsdata.sum_n) + fixed_half;
  ymin = lsdata.x_min * lsdata.m + lsdata.b;
  ymax = lsdata.x_max * lsdata.m + lsdata.b;

  xmin = (xmin) * xscale + fixed(rc.left + PaddingLeft);
  xmax = (xmax) * xscale + fixed(rc.left + PaddingLeft);
  ymin = (y_max - ymin * yscale) + fixed(rc.top);
  ymax = (y_max - ymax * yscale) + fixed(rc.top);

  RasterPoint line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(line[0], line[1], Style);
}

void
Chart::DrawLine(const fixed xmin, const fixed ymin,
                const fixed xmax, const fixed ymax, const Pen &pen)
{
  if (unscaled_x || unscaled_y)
    return;

  RasterPoint line[2];
  line[0].x = (int)((xmin - x_min) * xscale) + rc.left + PaddingLeft;
  line[0].y = (int)((y_max - ymin) * yscale) + rc.top;
  line[1].x = (int)((xmax - x_min) * xscale) + rc.left + PaddingLeft;
  line[1].y = (int)((y_max - ymax) * yscale) + rc.top;

  StyleLine(line[0], line[1], pen);
}

void 
Chart::DrawFilledLine(const fixed xmin, const fixed ymin,
                      const fixed xmax, const fixed ymax,
                      const Brush &brush)
{
  RasterPoint line[4];

  line[0].x = (int)((xmin - x_min) * xscale) + rc.left + PaddingLeft;
  line[0].y = (int)((y_max - ymin) * yscale) + rc.top;
  line[1].x = (int)((xmax - x_min) * xscale) + rc.left + PaddingLeft;
  line[1].y = (int)((y_max - ymax) * yscale) + rc.top;
  line[2].x = line[1].x;
  line[2].y = (int)((y_max) * yscale) + rc.top;
  line[3].x = line[0].x;
  line[3].y = (int)((y_max) * yscale) + rc.top;

  canvas.select(brush);
  canvas.null_pen();
  canvas.TriangleFan(line, 4);
}

void
Chart::DrawLine(const fixed xmin, const fixed ymin,
                const fixed xmax, const fixed ymax, ChartLook::Style Style)
{
  DrawLine(xmin, ymin, xmax, ymax, look.GetPen(Style));
}

void
Chart::DrawBarChart(const LeastSquares &lsdata)
{
  if (unscaled_x || unscaled_y)
    return;

  Brush green_brush(COLOR_GREEN);
  canvas.select(green_brush);
  canvas.null_pen();

  for (int i = 0; i < lsdata.sum_n; i++) {
    PixelScalar xmin((fixed(i) + fixed(1.2)) * xscale
                     + fixed(rc.left + PaddingLeft));
    PixelScalar ymin((y_max - y_min) * yscale + fixed(rc.top));
    PixelScalar xmax((fixed(i) + fixed(1.8)) * xscale
                     + fixed(rc.left + PaddingLeft));
    PixelScalar ymax((y_max - lsdata.ystore[i]) * yscale + fixed(rc.top));
    canvas.rectangle(xmin, ymin, xmax, ymax);
  }
}

void
Chart::DrawFilledLineGraph(const LeastSquares &lsdata)
{
  RasterPoint line[4];

  for (int i = 0; i < lsdata.sum_n - 1; i++) {
    line[0].x = (int)((lsdata.xstore[i] - x_min) * xscale) + rc.left + PaddingLeft;
    line[0].y = (int)((y_max - lsdata.ystore[i]) * yscale) + rc.top;
    line[1].x = (int)((lsdata.xstore[i + 1] - x_min) * xscale) + rc.left + PaddingLeft;
    line[1].y = (int)((y_max - lsdata.ystore[i + 1]) * yscale) + rc.top;
    line[2].x = line[1].x;
    line[2].y = rc.bottom - PaddingBottom;
    line[3].x = line[0].x;
    line[3].y = rc.bottom - PaddingBottom;
    canvas.TriangleFan(line, 4);
  }
}

void
Chart::DrawLineGraph(const LeastSquares &lsdata, const Pen &pen)
{
  RasterPoint line[2];

  for (int i = 0; i < lsdata.sum_n - 1; i++) {
    line[0].x = (int)((lsdata.xstore[i] - x_min) * xscale) + rc.left + PaddingLeft;
    line[0].y = (int)((y_max - lsdata.ystore[i]) * yscale) + rc.top;
    line[1].x = (int)((lsdata.xstore[i + 1] - x_min) * xscale) + rc.left + PaddingLeft;
    line[1].y = (int)((y_max - lsdata.ystore[i + 1]) * yscale) + rc.top;

    // STYLE_DASHGREEN
    // STYLE_MEDIUMBLACK
    StyleLine(line[0], line[1], pen);
  }
}

void
Chart::DrawLineGraph(const LeastSquares &lsdata, ChartLook::Style Style)
{
  DrawLineGraph(lsdata, look.GetPen(Style));
}

void
Chart::FormatTicText(TCHAR *text, const fixed val, const fixed step)
{
  if (step < fixed_one) {
    _stprintf(text, _T("%.1f"), (double)val);
  } else {
    _stprintf(text, _T("%.0f"), (double)val);
  }
}

void
Chart::DrawXGrid(const fixed tic_step, const fixed zero, ChartLook::Style Style,
                 const fixed unit_step, bool draw_units)
{
  DrawXGrid(tic_step, zero, look.GetPen(Style), unit_step, draw_units);
}

void
Chart::DrawXGrid(fixed tic_step, const fixed zero, const Pen &pen,
                 fixed unit_step, bool draw_units)
{
  if (!positive(tic_step))
    return;

  RasterPoint line[2];

  PixelScalar xmin, ymin, xmax, ymax;

  /** the minimum next position of the text, to avoid overlapping */
  PixelScalar next_text = rc.left;

  /* increase tic step so graph not too crowded */
  while ((x_max-x_min)/tic_step > fixed_ten) {
    tic_step *= fixed_two;
    unit_step *= fixed_two;
  }
  //  bool do_units = ((x_max-zero)/tic_step)<10;

  ymin = rc.top;
  ymax = rc.bottom;
  line[0].y = ymin;
  line[1].y = ymax - PaddingBottom;

  for (fixed xval = zero; xval <= x_max; xval += tic_step) {
    xmin = (int)((xval - x_min) * xscale) + rc.left + PaddingLeft;
    xmax = xmin;
    line[0].x = xmin;
    line[1].x = xmax;

    // STYLE_THINDASHPAPER
    if ((xval < x_max) && (xmin >= rc.left + PaddingLeft) && (xmin <= rc.right)) {
      StyleLine(line[0], line[1], pen);

      if (draw_units && xmin >= next_text) {
        TCHAR unit_text[MAX_PATH];
        FormatTicText(unit_text, xval * unit_step / tic_step, unit_step);

        canvas.background_transparent();
        canvas.text(xmin, ymax - Layout::Scale(17), unit_text);

        next_text = xmin + canvas.text_size(unit_text).cx + Layout::FastScale(2);
      }
    }
  }

  ymin = rc.top;
  ymax = rc.bottom;
  line[0].y = ymin;
  line[1].y = ymax - PaddingBottom;

  for (fixed xval = zero - tic_step; xval >= x_min; xval -= tic_step) {
    xmin = (int)((xval - x_min) * xscale) + rc.left + PaddingLeft;
    xmax = xmin;
    line[0].x = xmin;
    line[1].x = xmax;

    // STYLE_THINDASHPAPER

    if ((xval > x_min) && (xmin >= rc.left + PaddingLeft) && (xmin <= rc.right)) {
      StyleLine(line[0], line[1], pen);

      if (draw_units) {
        TCHAR unit_text[MAX_PATH];
        FormatTicText(unit_text, xval * unit_step / tic_step, unit_step);

        canvas.background_transparent();
        canvas.text(xmin, ymax - Layout::Scale(17), unit_text);
      }
    }
  }
}

void
Chart::DrawYGrid(const fixed tic_step, const fixed zero, ChartLook::Style Style,
                 const fixed unit_step, bool draw_units)
{
  DrawYGrid(tic_step, zero, look.GetPen(Style), unit_step, draw_units);
}

void
Chart::DrawYGrid(fixed tic_step, const fixed zero, const Pen &pen,
                 fixed unit_step, bool draw_units)
{
  if (!positive(tic_step))
    return;

  RasterPoint line[2];

  PixelScalar xmin, ymin, xmax, ymax;

  /* increase tic step so graph not too crowded */
  while ((y_max-y_min)/tic_step > fixed_ten) {
    tic_step *= fixed_two;
    unit_step *= fixed_two;
  }

  xmin = rc.left;
  xmax = rc.right;
  line[0].x = xmin + PaddingLeft;
  line[1].x = xmax;

  for (fixed yval = zero; yval <= y_max; yval += tic_step) {
    ymin = (int)((y_max - yval) * yscale) + rc.top;
    ymax = ymin;
    line[0].y = ymin;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    if ((yval < y_max) && (ymin >= rc.top) && (ymin <= rc.bottom - PaddingBottom)) {
      StyleLine(line[0], line[1], pen);

      if (draw_units) {
        TCHAR unit_text[MAX_PATH];
        FormatTicText(unit_text, yval * unit_step / tic_step, unit_step);

        canvas.background_transparent();
        canvas.text(xmin + Layout::Scale(8), ymin, unit_text);
      }
    }
  }

  xmin = rc.left;
  xmax = rc.right;
  line[0].x = xmin + PaddingLeft;
  line[1].x = xmax;

  for (fixed yval = zero - tic_step; yval >= y_min; yval -= tic_step) {
    ymin = (int)((y_max - yval) * yscale) + rc.top;
    ymax = ymin;
    line[0].y = ymin;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    if ((yval > y_min) && (ymin >= rc.top) && (ymin <= rc.bottom - PaddingBottom)) {
      StyleLine(line[0], line[1], pen);

      if (draw_units) {
        TCHAR unit_text[MAX_PATH];
        FormatTicText(unit_text, yval * unit_step / tic_step, unit_step);

        canvas.background_transparent();
        canvas.text(xmin + Layout::Scale(8), ymin, unit_text);
      }
    }
  }
}

void
Chart::ScaleMakeSquare()
{
  if (y_max <= y_min)
    return;

  if (rc.bottom - rc.top - PaddingBottom <= 0)
    return;

  fixed ar = fixed(rc.right - rc.left - PaddingLeft)
              / (rc.bottom - rc.top - PaddingBottom);
  fixed ard = (x_max - x_min) / (y_max - y_min);
  fixed armod = ard / ar;

  fixed delta;

  if (armod < fixed_one) {
    // need to expand width
    delta = (x_max - x_min) * (fixed_one / armod - fixed_one);
    x_max += delta / 2;
    x_min -= delta / 2;
  } else {
    // need to expand height
    delta = (y_max - y_min) * (armod - fixed_one);
    y_max += delta / 2;
    y_min -= delta / 2;
  }

  // shrink both by 10%
  delta = (x_max - x_min) / 10;
  x_max += delta / 2;
  x_min -= delta / 2;
  delta = (y_max - y_min) / 10;
  y_max += delta / 2;
  y_min -= delta / 2;

  yscale = fixed(rc.bottom - rc.top - PaddingBottom) / (y_max - y_min);
  xscale = fixed(rc.right - rc.left - PaddingLeft) / (x_max - x_min);
}

long
Chart::screenX(fixed x) const
{
  return (long)((x - x_min) * xscale) + rc.left + PaddingLeft;
}

long
Chart::screenY(fixed y) const
{
  return (long)((y_max - y) * yscale) + rc.top;
}

long
Chart::screenS(fixed s) const
{
  return (long)(s * yscale);
}

void
Chart::DrawArrow(const fixed x, const fixed y, const fixed mag,
                 const Angle angle, ChartLook::Style Style)
{
  RasterPoint wv[2];

  wv[0].x = screenX(x);
  wv[0].y = screenY(y);

  const FastRotation r(angle);
  FastRotation::Pair p;

  p = r.Rotate(mag, fixed_zero);
  wv[1].x = wv[0].x + (int)p.first;
  wv[1].y = wv[0].y + (int)p.second;
  StyleLine(wv[0], wv[1], Style);

  p = r.Rotate(mag - fixed(5), fixed(-3));
  wv[1].x = wv[0].x + (int)p.first;
  wv[1].y = wv[0].y + (int)p.second;
  StyleLine(wv[0], wv[1], Style);

  p = r.Rotate(mag - fixed(5), fixed(3));
  wv[1].x = wv[0].x + (int)p.first;
  wv[1].y = wv[0].y + (int)p.second;
  StyleLine(wv[0], wv[1], Style);
}

void 
Chart::DrawFilledY(const std::vector< std::pair<fixed, fixed> > &vals, 
                   const Brush &brush, const Pen* pen)
{
  if (vals.size()<2)
    return;
  const unsigned fsize = vals.size()+2;
  RasterPoint line[fsize];

  for (unsigned i=0; i< vals.size(); ++i) {
    line[i+2].x = (int)((vals[i].first - x_min) * xscale) + rc.left + PaddingLeft;
    line[i+2].y = (int)((y_max - vals[i].second) * yscale) + rc.top;
  }
  line[0].x = rc.left + PaddingLeft;
  line[0].y = line[fsize-1].y;
  line[1].x = rc.left + PaddingLeft;
  line[1].y = line[2].y;
  
  canvas.select(brush);
  if (pen == NULL) {
    canvas.null_pen();
  } else {
    canvas.select(*pen);
  }
  canvas.polygon(line, fsize);
}

void
Chart::DrawDot(const fixed x, const fixed y, const PixelScalar width)
{
  RasterPoint p;
  p.x = (PixelScalar)((x - x_min) * xscale) + rc.left + PaddingLeft;
  p.y = (PixelScalar)((y_max - y) * yscale) + rc.top;
  RasterPoint line[4] = { { p.x, PixelScalar(p.y - width) },
                          { PixelScalar(p.x - width), p.y },
                          { p.x, PixelScalar(p.y + width) },
                          { PixelScalar(p.x + width), p.y } };
  canvas.null_pen();
  canvas.TriangleFan(line, 4);
}
