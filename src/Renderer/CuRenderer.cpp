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

#include "CuRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Atmosphere/CuSonde.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Util/StringFormat.hpp"

#include <algorithm>

using std::min;
using std::max;

void
RenderTemperatureChart(Canvas &canvas, const PixelRect rc,
                       const ChartLook &chart_look,
                       const CuSonde &cu_sonde)
{
  ChartRenderer chart(chart_look, canvas, rc);

  int hmin = 10000;
  int hmax = -10000;
  auto tmin = cu_sonde.max_ground_temperature;
  auto tmax = cu_sonde.max_ground_temperature;

  // find range for scaling of graph
  for (unsigned i = 0; i < cu_sonde.NUM_LEVELS - 1u; i++) {
    if (cu_sonde.cslevels[i].empty())
      continue;

    hmin = min(hmin, (int)i);
    hmax = max(hmax, (int)i);

    auto dry_temperature = cu_sonde.cslevels[i].dry_temperature;
    auto air_temperature = cu_sonde.cslevels[i].air_temperature;

    tmin = min(tmin, min(dry_temperature, air_temperature));
    tmax = max(tmax, max(dry_temperature, air_temperature));

    if (!cu_sonde.cslevels[i].dewpoint_empty()) {
      auto dewpoint = cu_sonde.cslevels[i].dewpoint;
      tmin = min(tmin, dewpoint);
      tmax = max(tmax, dewpoint);
    }
  }

  if (hmin >= hmax) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleYFromValue(hmin * CuSonde::HEIGHT_STEP);
  chart.ScaleYFromValue(0);
  chart.ScaleYFromValue(hmax * CuSonde::HEIGHT_STEP);
  chart.ScaleXFromValue(tmin.ToUser());
  chart.ScaleXFromValue(tmax.ToUser());

  chart.DrawXGrid(5, 5, ChartRenderer::UnitFormat::NUMERIC);
  chart.DrawYGrid(Units::ToSysAltitude(500), 500, ChartRenderer::UnitFormat::NUMERIC);

  bool labelDry = false;
  bool labelAir = false;
  bool labelDew = false;

  int ipos = 0;

  for (unsigned i = 0; i < cu_sonde.NUM_LEVELS - 1u; i++) {
    bool has_dewpoint = false;

    if (cu_sonde.cslevels[i].empty() ||
        cu_sonde.cslevels[i + 1].empty())
      continue;

    auto alt = i * CuSonde::HEIGHT_STEP;
    auto next_alt = (i + 1) * CuSonde::HEIGHT_STEP;

    ipos++;

    chart.DrawLine(cu_sonde.cslevels[i].dry_temperature.ToUser(), alt,
                   cu_sonde.cslevels[i + 1].dry_temperature.ToUser(), next_alt,
                   ChartLook::STYLE_REDTHICKDASH);

    chart.DrawLine(cu_sonde.cslevels[i].air_temperature.ToUser(), alt,
                   cu_sonde.cslevels[i + 1].air_temperature.ToUser(), next_alt,
                   ChartLook::STYLE_BLACK);

    if (!cu_sonde.cslevels[i].dewpoint_empty() &&
        !cu_sonde.cslevels[i + 1].dewpoint_empty()) {
      chart.DrawLine(cu_sonde.cslevels[i].dewpoint.ToUser(), alt,
                     cu_sonde.cslevels[i + 1].dewpoint.ToUser(), next_alt,
                     ChartLook::STYLE_BLUETHINDASH);
               
      has_dewpoint = true;
    }      

    if (ipos > 2) {
      if (!labelDry) {
        chart.DrawLabel(_T("DALR"),
                        cu_sonde.cslevels[i + 1].dry_temperature.ToUser(), alt);
        labelDry = true;
      } else if (!labelAir) {
        chart.DrawLabel(_T("Air"),
                        cu_sonde.cslevels[i + 1].air_temperature.ToUser(), alt);
        labelAir = true;
      } else if (!labelDew && has_dewpoint) {
        chart.DrawLabel(_T("Dew"),
                        cu_sonde.cslevels[i + 1].dewpoint.ToUser(), alt);
        labelDew = true;
      }
    }
  }

  chart.DrawXLabel(_T("T"), Units::GetTemperatureName());
  chart.DrawYLabel(_T("h"), Units::GetAltitudeName());
}

void
TemperatureChartCaption(TCHAR *sTmp, const CuSonde &cu_sonde)
{
  StringFormatUnsafe(sTmp, _T("%s:\r\n  %5.0f %s\r\n\r\n%s:\r\n  %5.0f %s\r\n"),
                     _("Thermal height"),
                     (double)Units::ToUserAltitude(cu_sonde.thermal_height),
                     Units::GetAltitudeName(),
                     _("Cloud base"),
                     (double)Units::ToUserAltitude(cu_sonde.cloud_base),
                     Units::GetAltitudeName());
}
