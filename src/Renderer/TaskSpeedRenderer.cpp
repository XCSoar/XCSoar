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

#include "TaskSpeedRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
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

  if (!fs.task_speed.HasResult() || !task.CheckOrderedTask()) {
    chart.DrawNoData();
    return;
  }

  const float vref = glide_polar.GetAverageSpeed();

  chart.ScaleXFromData(fs.task_speed);
  chart.ScaleYFromData(fs.task_speed);
  chart.ScaleYFromValue(0);
  chart.ScaleYFromValue(vref);
  chart.ScaleXFromValue(fs.task_speed.GetMinX());
  if (derived_info.flight.flying)
    chart.ScaleXFromValue(derived_info.flight.flight_time/3600);

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

  chart.DrawLine(chart.GetXMin(), vref,
                 chart.GetXMax(), vref,
                 ChartLook::STYLE_REDTHICKDASH);

  chart.DrawLineGraph(fs.task_speed, ChartLook::STYLE_BLACK);
  chart.DrawTrend(fs.task_speed, ChartLook::STYLE_BLUETHINDASH);

  chart.DrawLabel(_T("Vest"),
                  chart.GetXMin()*0.9+chart.GetXMax()*0.1,
                  vref);

  const double tref = chart.GetXMin()*0.5+chart.GetXMax()*0.5;
  chart.DrawLabel(_T("Vave"),
                  tref,
                  fs.task_speed.GetYAt(tref));

  chart.DrawXLabel(_T("t"), _T("hr"));
  chart.DrawYLabel(_T("V"), Units::GetTaskSpeedName());
}
