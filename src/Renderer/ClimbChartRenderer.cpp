/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

void
ClimbChartCaption(TCHAR *sTmp,
                  const FlightStatistics &fs)
{
  ScopeLock lock(fs.mutexStats);
  if (fs.ThermalAverage.IsEmpty()) {
    sTmp[0] = _T('\0');
  } else if (fs.ThermalAverage.sum_n == 1) {
    _stprintf(sTmp, _T("%s:\r\n  %3.1f %s"),
              _("Avg. climb"),
              (double)Units::ToUserVSpeed(fixed(fs.ThermalAverage.y_ave)),
              Units::GetVerticalSpeedName());
  } else {
    _stprintf(sTmp, _T("%s:\r\n  %3.1f %s\r\n\r\n%s:\r\n  %3.2f %s"),
              _("Avg. climb"),
              (double)Units::ToUserVSpeed(fixed(fs.ThermalAverage.y_ave)),
              Units::GetVerticalSpeedName(),
              _("Climb trend"),
              (double)Units::ToUserVSpeed(fixed(fs.ThermalAverage.m)),
              Units::GetVerticalSpeedName());
  }
}

void
RenderClimbChart(Canvas &canvas, const PixelRect rc,
                 const ChartLook &chart_look,
                 const FlightStatistics &fs,
                 const GlidePolar &glide_polar)
{
  ChartRenderer chart(chart_look, canvas, rc);

  if (fs.ThermalAverage.IsEmpty()) {
    chart.DrawNoData();
    return;
  }

  fixed MACCREADY = glide_polar.GetMC();

  chart.ScaleYFromData(fs.ThermalAverage);
  chart.ScaleYFromValue(MACCREADY + fixed_half);
  chart.ScaleYFromValue(fixed_zero);

  chart.ScaleXFromValue(fixed_minus_one);
  chart.ScaleXFromValue(fixed(fs.ThermalAverage.sum_n));

  chart.DrawYGrid(Units::ToSysVSpeed(fixed_one), fixed_zero,
                  ChartLook::STYLE_THINDASHPAPER, fixed_one, true);
  chart.DrawBarChart(fs.ThermalAverage);

  chart.DrawLine(fixed_zero, MACCREADY, fixed(fs.ThermalAverage.sum_n), MACCREADY,
                 ChartLook::STYLE_REDTHICK);

  chart.DrawLabel(_T("MC"),
                  max(fixed_half, fixed(fs.ThermalAverage.sum_n) - fixed_one),
                  MACCREADY);

  chart.DrawTrendN(fs.ThermalAverage, ChartLook::STYLE_BLUETHIN);

  chart.DrawXLabel(_T("n"));
  chart.DrawYLabel(_T("w"), Units::GetVerticalSpeedName());
}
