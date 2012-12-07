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
  ScopeLock lock(fs.mutex);
  if (fs.thermal_average.IsEmpty()) {
    sTmp[0] = _T('\0');
  } else if (fs.thermal_average.sum_n == 1) {
    _stprintf(sTmp, _T("%s:\r\n  %3.1f %s"),
              _("Avg. climb"),
              (double)Units::ToUserVSpeed(fixed(fs.thermal_average.y_ave)),
              Units::GetVerticalSpeedName());
  } else {
    _stprintf(sTmp, _T("%s:\r\n  %3.1f %s\r\n\r\n%s:\r\n  %3.2f %s"),
              _("Avg. climb"),
              (double)Units::ToUserVSpeed(fixed(fs.thermal_average.y_ave)),
              Units::GetVerticalSpeedName(),
              _("Climb trend"),
              (double)Units::ToUserVSpeed(fixed(fs.thermal_average.m)),
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

  if (fs.thermal_average.IsEmpty()) {
    chart.DrawNoData();
    return;
  }

  fixed MACCREADY = glide_polar.GetMC();

  chart.ScaleYFromData(fs.thermal_average);
  chart.ScaleYFromValue(MACCREADY + fixed(0.5));
  chart.ScaleYFromValue(fixed(0));

  chart.ScaleXFromValue(fixed(-1));
  chart.ScaleXFromValue(fixed(fs.thermal_average.sum_n));

  chart.DrawYGrid(Units::ToSysVSpeed(fixed(1)), fixed(0),
                  ChartLook::STYLE_THINDASHPAPER, fixed(1), true);
  chart.DrawBarChart(fs.thermal_average);

  chart.DrawLine(fixed(0), MACCREADY, fixed(fs.thermal_average.sum_n), MACCREADY,
                 ChartLook::STYLE_REDTHICK);

  chart.DrawLabel(_T("MC"),
                  max(fixed(0.5), fixed(fs.thermal_average.sum_n) - fixed(1)),
                  MACCREADY);

  chart.DrawTrendN(fs.thermal_average, ChartLook::STYLE_BLUETHIN);

  chart.DrawXLabel(_T("n"));
  chart.DrawYLabel(_T("w"), Units::GetVerticalSpeedName());
}
