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

#include "CrossSectionRenderer.hpp"
#include "Renderer/ChartRenderer.hpp"
#include "Renderer/GradientRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Look/CrossSectionLook.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "MapSettings.hpp"
#include "Units/Units.hpp"
#include "NMEA/Aircraft.hpp"
#include "Navigation/Aircraft.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Language/Language.hpp"

CrossSectionRenderer::CrossSectionRenderer(const CrossSectionLook &_look,
                                           const AirspaceLook &_airspace_look,
                                           const ChartLook &_chart_look,
                                           const bool &_inverse)
    :inverse(_inverse), look(_look), chart_look(_chart_look), airspace_renderer(_airspace_look),
   terrain_renderer(look), terrain(NULL), airspace_database(NULL),
   start(GeoPoint::Invalid()),
   vec(50000, Angle::Zero()) {}

void
CrossSectionRenderer::ReadBlackboard(const MoreData &_gps_info,
                                     const DerivedInfo &_calculated_info,
                                     const GlideSettings &_glide_settings,
                                     const GlidePolar &_glide_polar,
                                     const MapSettings &map_settings)
{
  gps_info = _gps_info;
  calculated_info = _calculated_info;
  glide_settings = _glide_settings;
  glide_polar = _glide_polar;
  airspace_renderer.SetSettings(map_settings.airspace);
}

void
CrossSectionRenderer::Paint(Canvas &canvas, const PixelRect rc) const
{
  ChartRenderer chart(chart_look, canvas, rc);

  if (!vec.IsValid() || !start.IsValid()) {
    chart.DrawNoData(_("Not moving"));
    return;
  }

  DrawVerticalGradient(canvas, chart.GetChartRect(),
                       look.sky_color, look.background_color,
                       look.background_color);

  const auto nav_altitude = gps_info.NavAltitudeAvailable()
    ? gps_info.nav_altitude
    : 0.;
  auto hmin = fdim(nav_altitude, 3300);
  auto hmax = std::max(3300., nav_altitude + 1000.);

  chart.ResetScale();
  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(vec.distance);
  chart.ScaleYFromValue(hmin);
  chart.ScaleYFromValue(hmax);

  TerrainHeight elevations[NUM_SLICES];
  UpdateTerrain(elevations);

  if (airspace_database != nullptr) {
    const AircraftState aircraft = ToAircraftState(Basic(), Calculated());
    airspace_renderer.Draw(canvas, chart, *airspace_database, start, vec,
                           aircraft);
  }

  terrain_renderer.Draw(canvas, chart, elevations);
  PaintWorking(chart);
  PaintGlide(chart);
  PaintAircraft(canvas, chart, rc);

  canvas.SetTextColor(inverse? COLOR_WHITE: look.text_color);
  canvas.Select(*look.grid_font);

  PaintGrid(canvas, chart);
}

void
CrossSectionRenderer::UpdateTerrain(TerrainHeight *elevations) const
{
  if (terrain == NULL) {
    const auto invalid = TerrainHeight::Invalid();
    std::fill_n(elevations, NUM_SLICES, invalid);
    return;
  }

  const GeoPoint point_diff = vec.EndPoint(start) - start;

  RasterTerrain::Lease map(*terrain);
  for (unsigned i = 0; i < NUM_SLICES; ++i) {
    const auto slice_distance_factor = double(i) / (NUM_SLICES - 1);
    const GeoPoint slice_point = start + point_diff * slice_distance_factor;

    elevations[i] = map->GetHeight(slice_point);
  }
}

void
CrossSectionRenderer::PaintGlide(ChartRenderer &chart) const
{
  if (!gps_info.NavAltitudeAvailable() || !glide_polar.IsValid())
    return;

  const auto altitude = gps_info.nav_altitude;

  const MacCready mc(glide_settings, glide_polar);
  const GlideState task(vec, 0, altitude,
                        calculated_info.GetWindOrZero());
  const GlideResult result = mc.SolveStraight(task);
  if (!result.IsOk())
    return;

  // draw glide line if above zero
  if (result.GetArrivalAltitude()> 0.) {
    chart.DrawLine(0, altitude, result.vector.distance,
                   result.GetArrivalAltitude(),
                   ChartLook::STYLE_BLUE);
  } else {
    // draw glide line to zero
    const auto dh = altitude - result.GetArrivalAltitude();
    if (dh > 0.) {
      // proportion of distance to intercept zero
      const double p = altitude / dh;

      chart.DrawLine(0, altitude,
                     result.vector.distance * p, 0,
                     ChartLook::STYLE_BLUE);
    }
  }
}

void
CrossSectionRenderer::PaintWorking(ChartRenderer &chart) const
{
  const auto &h_max = calculated_info.common_stats.height_max_working;
  if ((h_max> chart.GetYMin()) && (h_max< chart.GetYMax())) {
    chart.DrawLine(0, h_max, chart.GetXMax(), h_max, ChartLook::STYLE_BLUETHINDASH);
  }
  const auto &h_min = calculated_info.common_stats.height_min_working;
  if ((h_min> chart.GetYMin()) && (h_min< chart.GetYMax())) {
    chart.DrawLine(0, h_min, chart.GetXMax(), h_min, ChartLook::STYLE_BLUETHINDASH);
  }
}

void
CrossSectionRenderer::PaintAircraft(Canvas &canvas, const ChartRenderer &chart,
                                    const PixelRect rc) const
{
  if (!gps_info.NavAltitudeAvailable())
    return;

  canvas.Select(look.aircraft_brush);
  canvas.SelectNullPen();

  BulkPixelPoint line[4];
  line[0] = chart.ToScreen(0, gps_info.nav_altitude);
  line[1].x = rc.left;
  line[1].y = line[0].y;
  line[2].x = line[1].x;
  line[2].y = line[0].y - (line[0].x - line[1].x) / 2;
  line[3].x = (line[1].x + line[0].x) / 2;
  line[3].y = line[0].y;
  canvas.DrawTriangleFan(line, 4);
}

void
CrossSectionRenderer::PaintGrid(Canvas &canvas, ChartRenderer &chart) const
{
  canvas.SetTextColor(inverse? COLOR_WHITE: look.text_color);

  chart.DrawXGrid(Units::ToSysDistance(5),5, ChartRenderer::UnitFormat::NUMERIC);
  chart.DrawYGrid(Units::ToSysAltitude(1000), 1000, ChartRenderer::UnitFormat::NUMERIC);

  chart.DrawXLabel(_T("D"), Units::GetDistanceName());
  chart.DrawYLabel(_T("h"), Units::GetAltitudeName());
}
