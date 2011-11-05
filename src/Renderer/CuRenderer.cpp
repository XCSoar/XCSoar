/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "FlightStatisticsRenderer.hpp"
#include "Atmosphere/CuSonde.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Screen/Chart.hpp"

#include <algorithm>
#include <stdio.h>

using std::min;
using std::max;

void
FlightStatisticsRenderer::RenderTemperature(Canvas &canvas,
                                            const PixelRect rc) const
{
  Chart chart(chart_look, canvas, rc);

  int hmin = 10000;
  int hmax = -10000;
  fixed tmin = CuSonde::maxGroundTemperature;
  fixed tmax = CuSonde::maxGroundTemperature;

  // find range for scaling of graph
  for (unsigned i = 0; i < CuSonde::NUM_LEVELS - 1u; i++) {
    if (CuSonde::cslevels[i].empty())
      continue;

    hmin = min(hmin, (int)i);
    hmax = max(hmax, (int)i);

    tmin = min(tmin, min(CuSonde::cslevels[i].tempDry,
                         min(CuSonde::cslevels[i].airTemp,
                             CuSonde::cslevels[i].dewpoint)));
    tmax = max(tmax, max(CuSonde::cslevels[i].tempDry,
                         max(CuSonde::cslevels[i].airTemp,
                             CuSonde::cslevels[i].dewpoint)));
  }

  if (hmin >= hmax) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleYFromValue(fixed(hmin));
  chart.ScaleYFromValue(fixed(hmax));
  chart.ScaleXFromValue(tmin);
  chart.ScaleXFromValue(tmax);

  bool labelDry = false;
  bool labelAir = false;
  bool labelDew = false;

  int ipos = 0;

  for (unsigned i = 0; i < CuSonde::NUM_LEVELS - 1u; i++) {
    if (CuSonde::cslevels[i].empty() ||
        CuSonde::cslevels[i + 1].empty())
      continue;

    ipos++;

    chart.DrawLine(CuSonde::cslevels[i].tempDry, fixed(i),
                   CuSonde::cslevels[i + 1].tempDry, fixed(i + 1),
                   ChartLook::STYLE_REDTHICK);

    chart.DrawLine(CuSonde::cslevels[i].airTemp, fixed(i),
                   CuSonde::cslevels[i + 1].airTemp, fixed(i + 1),
                   ChartLook::STYLE_MEDIUMBLACK);

    chart.DrawLine(CuSonde::cslevels[i].dewpoint, fixed(i),
                   CuSonde::cslevels[i + 1].dewpoint, fixed(i + 1),
                   ChartLook::STYLE_BLUETHIN);

    if (ipos > 2) {
      if (!labelDry) {
        chart.DrawLabel(_T("DALR"),
                        CuSonde::cslevels[i + 1].tempDry, fixed(i));
        labelDry = true;
      } else if (!labelAir) {
        chart.DrawLabel(_T("Air"),
                        CuSonde::cslevels[i + 1].airTemp, fixed(i));
        labelAir = true;
      } else if (!labelDew) {
        chart.DrawLabel(_T("Dew"),
                        CuSonde::cslevels[i + 1].dewpoint, fixed(i));
        labelDew = true;
      }
    }
  }

  chart.DrawXLabel(_T("T")_T(DEG));
  chart.DrawYLabel(_T("h"));
}

void
FlightStatisticsRenderer::CaptionTempTrace(TCHAR *sTmp) const
{
  _stprintf(sTmp, _T("%s:\r\n  %5.0f %s\r\n\r\n%s:\r\n  %5.0f %s\r\n"),
            _("Thermal height"),
            (double)Units::ToUserAltitude(CuSonde::thermalHeight),
            Units::GetAltitudeName(),
            _("Cloud base"),
            (double)Units::ToUserAltitude(CuSonde::cloudBase),
            Units::GetAltitudeName());
}
