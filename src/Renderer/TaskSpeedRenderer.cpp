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
#include "util/UTF8.hpp"

#include <fmt/format.h>

void
TaskSpeedCaption(char *s_tmp, size_t buffer_size,
                 const FlightStatistics &fs,
                 const GlidePolar &glide_polar)
{
  if (s_tmp == nullptr || buffer_size == 0)
    return;

  if (!glide_polar.IsValid() || fs.task_speed.IsEmpty()) {
    *s_tmp = '\0';
    return;
  }

  auto result = fmt::format_to_n(s_tmp, buffer_size - 1,
                                 "{}: {} {}\r\n{}: {} {}",
                                 C_("Average velocity abbreviation", "Vave"),
                                 (int)Units::ToUserTaskSpeed(fs.task_speed.GetAverageY()),
                                 Units::GetTaskSpeedName(),
                                 _("Vest"),
                                 (int)Units::ToUserTaskSpeed(glide_polar.GetAverageSpeed()),
                                 Units::GetTaskSpeedName());
  *result.out = '\0';
  CropIncompleteUTF8(s_tmp);
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
  chart.SetXLabel("t", "hr");
  chart.SetYLabel("V", Units::GetTaskSpeedName());
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
                         chart_look.color_positive, chart_look.background_color,
                         chart_look.background_color);
  }
  {
    PixelRect rc_lower = chart.GetChartRect();
    rc_lower.top = chart.ScreenY(vref);

    DrawVerticalGradient(canvas, rc_lower,
                         chart_look.background_color, chart_look.color_negative,
                         chart_look.background_color);
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
                  "Vest");

  const double tref = chart.GetXMin()*0.5+chart.GetXMax()*0.5;
  chart.DrawLabel({tref, fs.task_speed.GetYAt(tref)}, "Vave");

  chart.Finish();
}
