// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskSpeedRenderer.hpp"
#include "ChartRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Units/Units.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "FlightStatistics.hpp"
#include "Language/Language.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "TaskLegRenderer.hpp"
#include "GradientRenderer.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"

void
TaskSpeedCaption(TCHAR *sTmp,
                 const FlightStatistics &fs,
                 const GlidePolar &glide_polar)
{
  if (!glide_polar.IsValid() || fs.task_speed.IsEmpty()) {
    *sTmp = _T('\0');
    return;
  }

  _stprintf(sTmp,
            _T("%s: %d %s\r\n%s: %d %s"),
            _("Vave"),
            (int)Units::ToUserTaskSpeed(fs.task_speed.GetAverageY()),
            Units::GetTaskSpeedName(),
            _("Vest"),
            (int)Units::ToUserTaskSpeed(glide_polar.GetAverageSpeed()),
            Units::GetTaskSpeedName());
}

void
RenderSpeed(Canvas &canvas, const PixelRect rc,
            const ChartLook &chart_look,
            const FlightStatistics &fs,
            const NMEAInfo &nmea_info,
            const DerivedInfo &derived_info,
            const TaskManager &task,
            const GlidePolar &glide_polar)
{
  ChartRenderer chart(chart_look, canvas, rc);
  chart.SetXLabel(_T("t"), _T("hr"));
  chart.SetYLabel(_T("V"), Units::GetTaskSpeedName());
  chart.Begin();

  if (!fs.task_speed.HasResult() || !task.CheckOrderedTask()) {
    chart.DrawNoData();
    chart.Finish();
    return;
  }

  const float vref = glide_polar.GetAverageSpeed();

  chart.ScaleXFromData(fs.task_speed);
  chart.ScaleYFromData(fs.task_speed);
  chart.ScaleYFromValue(0);
  chart.ScaleYFromValue(vref);
  chart.ScaleXFromValue(fs.task_speed.GetMinX());
  if (derived_info.flight.flying)
    chart.ScaleXFromValue(derived_info.flight.flight_time / std::chrono::hours{1});

  // draw red area below average speed, blue area above
  {
    PixelRect rc_upper = chart.GetChartRect();
    rc_upper.bottom = chart.ScreenY(vref);

    DrawVerticalGradient(canvas, rc_upper,
                         chart_look.color_positive, COLOR_WHITE, COLOR_WHITE);
  }
  {
    PixelRect rc_lower = chart.GetChartRect();
    rc_lower.top = chart.ScreenY(vref);

    DrawVerticalGradient(canvas, rc_lower,
                         COLOR_WHITE, chart_look.color_negative, COLOR_WHITE);
  }

  RenderTaskLegs(chart, task, nmea_info, derived_info, 0.33);

  chart.DrawXGrid(0.25, 0.25, ChartRenderer::UnitFormat::TIME);
  chart.DrawYGrid(Units::ToSysTaskSpeed(10), 10, ChartRenderer::UnitFormat::NUMERIC);

  chart.DrawLine({chart.GetXMin(), vref},
                 {chart.GetXMax(), vref},
                 ChartLook::STYLE_REDTHICKDASH);

  chart.DrawLineGraph(fs.task_speed, ChartLook::STYLE_BLACK);
  chart.DrawTrend(fs.task_speed, ChartLook::STYLE_BLUETHINDASH);

  chart.DrawLabel({chart.GetXMin()*0.9+chart.GetXMax()*0.1, vref},
                  _T("Vest"));

  const double tref = chart.GetXMin()*0.5+chart.GetXMax()*0.5;
  chart.DrawLabel({tref, fs.task_speed.GetYAt(tref)}, _T("Vave"));

  chart.Finish();
}
