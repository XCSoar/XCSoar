// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ClimbChartRenderer.hpp"
#include "ChartRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Units/Units.hpp"
#include "FlightStatistics.hpp"
#include "Language/Language.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "util/StringFormat.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "TaskLegRenderer.hpp"
#include "GradientRenderer.hpp"

void
ClimbChartCaption(TCHAR *sTmp,
                  const FlightStatistics &fs)
{
  const std::lock_guard lock{fs.mutex};
  if (fs.thermal_average.IsEmpty()) {
    sTmp[0] = _T('\0');
  } else if (fs.thermal_average.GetCount() == 1) {
    StringFormatUnsafe(sTmp, _T("%s:\r\n  %3.1f %s"),
                       _("Avg. climb"),
                       (double)Units::ToUserVSpeed(fs.thermal_average.GetAverageY()),
                       Units::GetVerticalSpeedName());
  } else {
    StringFormatUnsafe(sTmp, _T("%s:\r\n  %3.1f %s\r\n\r\n%s:\r\n  %3.2f %s/hr"),
                       _("Avg. climb"),
                       (double)Units::ToUserVSpeed(fs.thermal_average.GetAverageY()),
                       Units::GetVerticalSpeedName(),
                       _("Climb trend"),
                       (double)Units::ToUserVSpeed(fs.thermal_average.GetGradient()),
                       Units::GetVerticalSpeedName());
  }
}

void
RenderClimbChart(Canvas &canvas, const PixelRect rc,
                 const ChartLook &chart_look,
                 const FlightStatistics &fs,
                 const GlidePolar &glide_polar,
                 const NMEAInfo &nmea_info,
                 const DerivedInfo &derived_info,
                 const TaskManager &task)
{
  ChartRenderer chart(chart_look, canvas, rc);
  chart.SetXLabel(_T("t"), _T("hr"));
  chart.SetYLabel(_T("w"), Units::GetVerticalSpeedName());
  chart.Begin();

  if (fs.thermal_average.IsEmpty()) {
    chart.DrawNoData();
    chart.Finish();
    return;
  }

  auto MACCREADY = glide_polar.GetMC();

  chart.ScaleYFromData(fs.thermal_average);
  chart.ScaleYFromValue(MACCREADY + 0.5);
  chart.ScaleYFromValue(0);
  chart.ScaleXFromData(fs.thermal_average);
  if (derived_info.flight.flying)
    chart.ScaleXFromValue(derived_info.flight.flight_time / std::chrono::hours{1});

  // draw red area below MC, blue area above
  {
    PixelRect rc_upper = chart.GetChartRect();
    rc_upper.bottom = chart.ScreenY(MACCREADY);

    DrawVerticalGradient(canvas, rc_upper,
                         chart_look.color_positive, COLOR_WHITE, COLOR_WHITE);
  }
  {
    PixelRect rc_lower = chart.GetChartRect();
    rc_lower.top = chart.ScreenY(MACCREADY);

    DrawVerticalGradient(canvas, rc_lower,
                         COLOR_WHITE, chart_look.color_negative, COLOR_WHITE);
  }

  RenderTaskLegs(chart, task, nmea_info, derived_info, 0.8);

  canvas.Select(chart_look.black_brush);
  chart.DrawWeightBarGraph(fs.thermal_average);

  chart.DrawXGrid(0.25, 0.25, ChartRenderer::UnitFormat::TIME);
  chart.DrawYGrid(Units::ToSysVSpeed(0.2), 0.2, ChartRenderer::UnitFormat::NUMERIC);

  chart.DrawTrend(fs.thermal_average, ChartLook::STYLE_BLUETHINDASH);

  chart.DrawLine({chart.GetXMin(), MACCREADY},
                 {chart.GetXMax(), MACCREADY},
                 ChartLook::STYLE_REDTHICKDASH);

  chart.DrawLabel({chart.GetXMin()*0.9+chart.GetXMax()*0.1, MACCREADY},
                  _T("MC"));

  chart.Finish();
}
