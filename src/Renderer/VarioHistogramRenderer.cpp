// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioHistogramRenderer.hpp"
#include "ChartRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "FlightStatistics.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "util/StaticString.hxx"
#include "GradientRenderer.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

void
RenderVarioHistogram(Canvas &canvas, const PixelRect rc,
                     const ChartLook &chart_look,
                     const FlightStatistics &fs,
                     const GlidePolar &glide_polar)
{
  ChartRenderer chart(chart_look, canvas, rc);
  chart.SetYLabel(_T("w"), Units::GetVerticalSpeedName());
  chart.Begin();

  if (fs.vario_cruise_histogram.empty() &&
      fs.vario_circling_histogram.empty()) {
    chart.DrawNoData();
    chart.Finish();
    return;
  }

  const auto scale = std::max(fs.vario_cruise_histogram.GetMaxY(),
                              fs.vario_circling_histogram.GetMaxY()) * 1.2;
  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(scale);

  const auto s = -glide_polar.GetSBestLD();
  const auto mc = glide_polar.GetMC();

  constexpr double y_padding = 0.5;

  chart.ScaleYFromValue(fs.vario_cruise_histogram.GetMinX() - y_padding);
  chart.ScaleYFromValue(fs.vario_cruise_histogram.GetMaxX() + y_padding);
  chart.ScaleYFromValue(fs.vario_circling_histogram.GetMinX() - y_padding);
  chart.ScaleYFromValue(fs.vario_circling_histogram.GetMaxX() + y_padding);
  chart.ScaleYFromValue(s - y_padding);
  chart.ScaleYFromValue(mc + y_padding);

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
    chart.DrawFilledLineGraph(fs.vario_circling_histogram.GetSlots(), true);
  }
  {
    canvas.Select(chart_look.blank_brush);
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    chart.DrawFilledLineGraph(fs.vario_cruise_histogram.GetSlots(), true);
  }

  // draw these after shaded regions, so they overlay
  chart.DrawLineGraph(fs.vario_cruise_histogram.GetSlots(), ChartLook::STYLE_BLUE, true);
  chart.DrawLineGraph(fs.vario_circling_histogram.GetSlots(), ChartLook::STYLE_RED, true);

  // draw current MC setting
  chart.DrawLine({0, mc}, {scale, mc}, ChartLook::STYLE_REDTHICKDASH);
  chart.DrawLine({0, s}, {scale, s}, ChartLook::STYLE_BLUETHINDASH);

  // draw labels and other overlays
  chart.DrawYGrid(Units::ToSysVSpeed(1), 1, ChartRenderer::UnitFormat::NUMERIC);

  const double tref = chart.GetXMin()*0.1+chart.GetXMax()*0.9;
  chart.DrawLabel({tref, mc}, _T("MC"));
  chart.DrawLabel({tref, s}, _T("S cruise"));

  chart.Finish();
}
