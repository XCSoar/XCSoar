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

#include "WindChartRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Computer/Wind/Store.hpp"
#include "FlightStatistics.hpp"
#include "NMEA/Info.hpp"
#include "Math/LeastSquares.hpp"
#include "Units/Units.hpp"
#include "Math/FastRotation.hpp"
#include "Screen/Canvas.hpp"

static void
DrawArrow(Canvas &canvas, RasterPoint point, const fixed mag, const Angle angle)
{
  const FastRotation r(angle);

  auto p = r.Rotate(mag, fixed(0));
  canvas.DrawLine(point, point + RasterPoint((int)p.x, (int)p.y));

  p = r.Rotate(mag - fixed(5), fixed(-3));
  canvas.DrawLine(point, point + RasterPoint((int)p.x, (int)p.y));

  p = r.Rotate(mag - fixed(5), fixed(3));
  canvas.DrawLine(point, point + RasterPoint((int)p.x, (int)p.y));
}

void
RenderWindChart(Canvas &canvas, const PixelRect rc,
                const ChartLook &chart_look,
                const FlightStatistics &fs,
                const NMEAInfo &nmea_info,
                const WindStore &wind_store)
{
  unsigned numsteps = 10;
  bool found = true;

  LeastSquares windstats_mag;

  ChartRenderer chart(chart_look, canvas, rc);

  const auto height =
    fs.altitude_ceiling.GetMaxY() - fs.altitude_ceiling.GetMinY();
  if (height <= fixed(10)) {
    chart.DrawNoData();
    return;
  }

  windstats_mag.Reset();

  for (unsigned i = 0; i < numsteps; i++) {
    auto h = height * i / (numsteps - 1) + fs.altitude_base.GetMinY();

    Vector wind = wind_store.GetWind(nmea_info.time, h, found);
    auto mag = wind.Magnitude();

    windstats_mag.Update(mag, h);
  }

  chart.ScaleXFromData(windstats_mag);
  chart.ScaleXFromValue(fixed(0));
  chart.ScaleXFromValue(fixed(10));

  chart.ScaleYFromData(windstats_mag);

  chart.DrawXGrid(Units::ToSysSpeed(fixed(5)),
                  ChartLook::STYLE_THINDASHPAPER, fixed(5), true);
  chart.DrawYGrid(Units::ToSysAltitude(fixed(1000)),
                  ChartLook::STYLE_THINDASHPAPER, fixed(1000), true);
  chart.DrawLineGraph(windstats_mag, ChartLook::STYLE_MEDIUMBLACK);

#define WINDVECTORMAG 25

  numsteps = (int)((rc.bottom - rc.top) / WINDVECTORMAG) - 1;

  canvas.Select(chart_look.GetPen(ChartLook::STYLE_MEDIUMBLACK));

  // draw direction vectors
  const auto x_max = std::max(windstats_mag.GetMaxX(),
                              fixed(1)); // prevent /0 problems
  fixed hfact;
  for (unsigned i = 0; i < numsteps; i++) {
    hfact = fixed(i + 1) / (numsteps + 1);
    auto h = height * hfact + fs.altitude_base.GetMinY();

    Vector wind = wind_store.GetWind(nmea_info.time, h, found);
    wind.x /= x_max;
    wind.y /= x_max;
    auto mag = wind.Magnitude();
    if (negative(mag))
      continue;

    Angle angle = Angle::FromXY(wind.y, -wind.x);

    RasterPoint point = chart.ToScreen((chart.GetXMin() + chart.GetXMax()) / 2, h);

    DrawArrow(canvas, point, mag * WINDVECTORMAG, angle);
  }

  chart.DrawXLabel(_T("w"), Units::GetSpeedName());
  chart.DrawYLabel(_T("h"), Units::GetAltitudeName());
}
