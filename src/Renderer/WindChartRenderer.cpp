// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindChartRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Computer/Wind/Store.hpp"
#include "FlightStatistics.hpp"
#include "NMEA/Info.hpp"
#include "Math/LeastSquares.hpp"
#include "Units/Units.hpp"
#include "Math/FastRotation.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"

static void
DrawArrow(Canvas &canvas, PixelPoint point, const double mag, const Angle angle)
{
  const FastRotation r(angle);

  auto p = r.Rotate({mag, 0});
  canvas.DrawLine(point, point + PixelPoint((int)p.x, (int)p.y));

  const int l = Layout::Scale(5);
  const int s = Layout::Scale(3);

  p = r.Rotate({mag - l, (double)-s});
  canvas.DrawLine(point, point + PixelPoint((int)p.x, (int)p.y));

  p = r.Rotate({mag - l, (double)s});
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
  chart.SetXLabel(_T("w"), Units::GetSpeedName());
  chart.SetYLabel(_T("h"), Units::GetAltitudeName());
  chart.Begin();

  if (fs.altitude_base.IsEmpty() || fs.altitude_ceiling.IsEmpty()) {
    chart.DrawNoData();
    chart.Finish();
    return;
  }

  const auto height =
    fs.altitude_ceiling.GetMaxY() - fs.altitude_ceiling.GetMinY();
  if (height <= 10) {
    chart.DrawNoData();
    chart.Finish();
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
  chart.DrawYGrid(Units::ToSysAltitude(250), 250, ChartRenderer::UnitFormat::NUMERIC);
  chart.DrawLineGraph(windstats_mag, ChartLook::STYLE_BLACK);

#define WINDVECTORMAG Layout::Scale(25)

  numsteps = (int)(rc.GetHeight() / WINDVECTORMAG) - 1;

  canvas.Select(chart_look.GetPen(ChartLook::STYLE_BLACK));

  // draw direction vectors
  const auto x_max = std::max(windstats_mag.GetMaxX(),
                              1.); // prevent /0 problems
  for (unsigned i = 0; i < numsteps; i++) {
    double hfact = double(i + 1) / (numsteps + 1);
    auto h = height * hfact + fs.altitude_base.GetMinY();

    Vector wind = wind_store.GetWind(nmea_info.time, h, found);
    wind.x /= x_max;
    wind.y /= x_max;
    auto mag = wind.Magnitude();
    if (mag < 0)
      continue;

    Angle angle = Angle::FromXY(wind.y, -wind.x);

    auto point = chart.ToScreen({(chart.GetXMin() + chart.GetXMax()) / 2, h});

    DrawArrow(canvas, point, mag * WINDVECTORMAG, angle);
  }

  chart.Finish();
}
