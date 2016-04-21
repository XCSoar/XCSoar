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

#include "VarioHistogramRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "FlightStatistics.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "Util/StaticString.hxx"

void
RenderVarioHistogram(Canvas &canvas, const PixelRect rc,
                     const ChartLook &chart_look,
                     const FlightStatistics &fs,
                     const GlidePolar &glide_polar)
{
  ChartRenderer chart(chart_look, canvas, rc);

  const auto acc = std::max(fs.vario_cruise_histogram.GetAccumulator(),
                            fs.vario_circling_histogram.GetAccumulator());

  if (!acc) {
    chart.DrawNoData();
    return;
  }

  const auto scale = std::max(fs.vario_cruise_histogram.GetMaxY(),
                              fs.vario_circling_histogram.GetMaxY()) * 1.2;

  chart.ScaleXFromValue(fs.vario_cruise_histogram.GetMinX()-0.5);
  chart.ScaleXFromValue(fs.vario_cruise_histogram.GetMaxX()+0.5);
  chart.ScaleYFromValue(0);
  chart.ScaleYFromValue(scale);
  const auto s = -glide_polar.GetSBestLD();
  const auto mc = glide_polar.GetMC();
  chart.ScaleXFromValue(s);
  chart.ScaleXFromValue(mc);

  Pen red_pen(2, COLOR_RED);
  Pen green_pen(2, COLOR_GREEN);

  canvas.SelectNullPen();
  canvas.Select(chart_look.bar_brush);
  chart.DrawFilledLineGraph(fs.vario_circling_histogram);
  canvas.Select(chart_look.neg_brush);
  chart.DrawFilledLineGraph(fs.vario_cruise_histogram);

  chart.DrawLineGraph(fs.vario_cruise_histogram, red_pen);
  chart.DrawLineGraph(fs.vario_circling_histogram, green_pen);

  // draw current MC setting
  chart.DrawLine(mc, 0, mc, scale, ChartLook::STYLE_REDTHICK);
  chart.DrawLine(s, 0, s, scale, ChartLook::STYLE_MEDIUMBLACK);

  // draw labels and other overlays
  chart.DrawXGrid(Units::ToSysVSpeed(1), 1, ChartRenderer::UnitFormat::NUMERIC);

  chart.DrawXLabel(_T("w"), Units::GetVerticalSpeedName());

}
