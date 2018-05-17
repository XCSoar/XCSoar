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
#include "Screen/Layout.hpp"

static void
DrawArrow(Canvas &canvas, PixelPoint point, const double mag, const Angle angle)
{
  const FastRotation r(angle);

  auto p = r.Rotate(mag, 0);
  canvas.DrawLine(point, point + PixelPoint((int)p.x, (int)p.y));

  const int l = Layout::Scale(5);
  const int s = Layout::Scale(3);

  p = r.Rotate(mag - l, -s);
  canvas.DrawLine(point, point + PixelPoint((int)p.x, (int)p.y));

  p = r.Rotate(mag - l, s);
  canvas.DrawLine(point, point + PixelPoint((int)p.x, (int)p.y));
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
  if (height <= 10) {
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
  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(10);

  chart.ScaleYFromData(windstats_mag);

  chart.DrawXGrid(Units::ToSysSpeed(5), 5, ChartRenderer::UnitFormat::NUMERIC);
  chart.DrawYGrid(Units::ToSysAltitude(1000), 1000, ChartRenderer::UnitFormat::NUMERIC);
  chart.DrawLineGraph(windstats_mag, ChartLook::STYLE_BLACK);

#define WINDVECTORMAG Layout::Scale(25)

  numsteps = (int)(rc.GetHeight() / WINDVECTORMAG) - 1;

  canvas.Select(chart_look.GetPen(ChartLook::STYLE_BLACK));

  // draw direction vectors
  const auto x_max = std::max(windstats_mag.GetMaxX(),
                              1.); // prevent /0 problems
  double hfact;
  for (unsigned i = 0; i < numsteps; i++) {
    hfact = double(i + 1) / (numsteps + 1);
    auto h = height * hfact + fs.altitude_base.GetMinY();

    Vector wind = wind_store.GetWind(nmea_info.time, h, found);
    wind.x /= x_max;
    wind.y /= x_max;
    auto mag = wind.Magnitude();
    if (mag < 0)
      continue;

    Angle angle = Angle::FromXY(wind.y, -wind.x);

    auto point = chart.ToScreen((chart.GetXMin() + chart.GetXMax()) / 2, h);

    DrawArrow(canvas, point, mag * WINDVECTORMAG, angle);
  }

  chart.DrawXLabel(_T("w"), Units::GetSpeedName());
  chart.DrawYLabel(_T("h"), Units::GetAltitudeName());
}
