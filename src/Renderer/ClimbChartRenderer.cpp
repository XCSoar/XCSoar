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

#include "ClimbChartRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Units/Units.hpp"
#include "FlightStatistics.hpp"
#include "Language/Language.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Util/StringFormat.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "TaskLegRenderer.hpp"
#include "GradientRenderer.hpp"

void
ClimbChartCaption(TCHAR *sTmp,
                  const FlightStatistics &fs)
{
  ScopeLock lock(fs.mutex);
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

  if (fs.thermal_average.IsEmpty()) {
    chart.DrawNoData();
    return;
  }

  auto MACCREADY = glide_polar.GetMC();

  chart.ScaleYFromData(fs.thermal_average);
  chart.ScaleYFromValue(MACCREADY + 0.5);
  chart.ScaleYFromValue(0);
  chart.ScaleXFromData(fs.thermal_average);
  if (derived_info.flight.flying)
    chart.ScaleXFromValue(derived_info.flight.flight_time/3600);

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
  chart.DrawYGrid(Units::ToSysVSpeed(1), 1, ChartRenderer::UnitFormat::NUMERIC);

  chart.DrawTrend(fs.thermal_average, ChartLook::STYLE_BLUETHINDASH);

  chart.DrawLine(chart.GetXMin(), MACCREADY,
                 chart.GetXMax(), MACCREADY,
                 ChartLook::STYLE_REDTHICKDASH);

  chart.DrawLabel(_T("MC"),
                  chart.GetXMin()*0.9+chart.GetXMax()*0.1,
                  MACCREADY);

  // draw labels and other overlays
  chart.DrawXLabel(_T("t"), _T("hr"));
  chart.DrawYLabel(_T("w"), Units::GetVerticalSpeedName());
}
