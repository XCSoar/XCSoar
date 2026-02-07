// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CuRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Atmosphere/CuSonde.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "util/StringFormat.hpp"

#include <algorithm>

using std::min;
using std::max;

void
RenderTemperatureChart(Canvas &canvas, const PixelRect rc,
                       const ChartLook &chart_look,
                       const CuSonde &cu_sonde)
{
  ChartRenderer chart(chart_look, canvas, rc);
  chart.SetXLabel("T", Units::GetTemperatureName());
  chart.SetYLabel("h", Units::GetAltitudeName());
  chart.Begin();

  int hmin = 10000;
  int hmax = -10000;
  auto tmin = cu_sonde.max_ground_temperature;
  auto tmax = cu_sonde.max_ground_temperature;

  // find range for scaling of graph
  for (unsigned i = 0; i < cu_sonde.cslevels.size() - 1u; i++) {
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
    chart.Finish();
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

  for (unsigned i = 0; i < cu_sonde.cslevels.size() - 1u; i++) {
    bool has_dewpoint = false;

    if (cu_sonde.cslevels[i].empty() ||
        cu_sonde.cslevels[i + 1].empty())
      continue;

    const double alt = i * CuSonde::HEIGHT_STEP;
    const double next_alt = (i + 1) * CuSonde::HEIGHT_STEP;

    ipos++;

    chart.DrawLine({cu_sonde.cslevels[i].dry_temperature.ToUser(), alt},
                   {cu_sonde.cslevels[i + 1].dry_temperature.ToUser(), next_alt},
                   ChartLook::STYLE_REDTHICKDASH);

    chart.DrawLine({cu_sonde.cslevels[i].air_temperature.ToUser(), alt},
                   {cu_sonde.cslevels[i + 1].air_temperature.ToUser(), next_alt},
                   ChartLook::STYLE_BLACK);

    if (!cu_sonde.cslevels[i].dewpoint_empty() &&
        !cu_sonde.cslevels[i + 1].dewpoint_empty()) {
      chart.DrawLine({cu_sonde.cslevels[i].dewpoint.ToUser(), alt},
                     {cu_sonde.cslevels[i + 1].dewpoint.ToUser(), next_alt},
                     ChartLook::STYLE_BLUETHINDASH);
               
      has_dewpoint = true;
    }      

    if (ipos > 2) {
      if (!labelDry) {
        chart.DrawLabel({cu_sonde.cslevels[i + 1].dry_temperature.ToUser(), alt},
                        "DALR");
        labelDry = true;
      } else if (!labelAir) {
        chart.DrawLabel({cu_sonde.cslevels[i + 1].air_temperature.ToUser(), alt},
                        "Air");
        labelAir = true;
      } else if (!labelDew && has_dewpoint) {
        chart.DrawLabel({cu_sonde.cslevels[i + 1].dewpoint.ToUser(), alt},
                        "Dew");
        labelDew = true;
      }
    }
  }

  chart.Finish();
}

void
TemperatureChartCaption(char *sTmp, const CuSonde &cu_sonde)
{
  StringFormatUnsafe(sTmp, "%s:\r\n  %5.0f %s\r\n\r\n%s:\r\n  %5.0f %s\r\n",
                     _("Thermal height"),
                     (double)Units::ToUserAltitude(cu_sonde.thermal_height),
                     Units::GetAltitudeName(),
                     _("Cloud base"),
                     (double)Units::ToUserAltitude(cu_sonde.cloud_base),
                     Units::GetAltitudeName());
}
