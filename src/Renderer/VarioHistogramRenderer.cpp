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
#include "GradientRenderer.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

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
  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(scale);

  const auto s = -glide_polar.GetSBestLD();
  const auto mc = glide_polar.GetMC();
  chart.ScaleYFromValue(fs.vario_cruise_histogram.GetMinX());
  chart.ScaleYFromValue(fs.vario_cruise_histogram.GetMaxX());
  chart.ScaleYFromValue(s);
  chart.ScaleYFromValue(mc);

  // draw red area at higher than cruise sink rate, blue area above mc
  {
    PixelRect rc_upper = chart.GetChartRect();
    rc_upper.bottom = chart.ScreenY(mc);

    DrawVerticalGradient(canvas, rc_upper,
                         chart_look.color_positive, COLOR_WHITE, COLOR_WHITE);
  }
  {
    PixelRect rc_lower = chart.GetChartRect();
    rc_lower.top = chart.ScreenY(s);

    DrawVerticalGradient(canvas, rc_lower,
                         COLOR_WHITE, chart_look.color_negative, COLOR_WHITE);
  }

  canvas.SelectNullPen();
  {
    canvas.Select(chart_look.black_brush);
    chart.DrawFilledLineGraph(fs.vario_circling_histogram, true);
  }
  {
    canvas.Select(chart_look.blank_brush);
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    chart.DrawFilledLineGraph(fs.vario_cruise_histogram, true);
  }

  // draw these after shaded regions, so they overlay
  chart.DrawLineGraph(fs.vario_cruise_histogram, ChartLook::STYLE_GREEN, true);
  chart.DrawLineGraph(fs.vario_circling_histogram, ChartLook::STYLE_RED, true);

  // draw current MC setting
  chart.DrawLine(0, mc, scale, mc, ChartLook::STYLE_REDTHICKDASH);
  chart.DrawLine(0, s, scale, s, ChartLook::STYLE_BLUETHINDASH);

  // draw labels and other overlays
  chart.DrawYGrid(Units::ToSysVSpeed(1), 1, ChartRenderer::UnitFormat::NUMERIC);

  const double tref = chart.GetXMin()*0.1+chart.GetXMax()*0.9;
  chart.DrawLabel(_T("MC"), tref, mc);
  chart.DrawLabel(_T("S cruise"), tref, s);

  chart.DrawYLabel(_T("w"), Units::GetVerticalSpeedName());

}
