/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Components.hpp"
#include "Interface.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Look/CrossSectionLook.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Units/Units.hpp"
#include "NMEA/Aircraft.hpp"
#include "Navigation/Aircraft.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

CrossSectionRenderer::CrossSectionRenderer(const CrossSectionLook &_look,
                                       const AirspaceLook &_airspace_look,
                                       const ChartLook &_chart_look)
  :look(_look), chart_look(_chart_look), airspace_renderer(_airspace_look),
  terrain_renderer(look), terrain(NULL), airspace_database(NULL),
  start(Angle::Zero(), Angle::Zero()),
   vec(fixed(50000), Angle::Zero()) {}

void
CrossSectionRenderer::ReadBlackboard(const MoreData &_gps_info,
                                   const DerivedInfo &_calculated_info,
                                   const AirspaceRendererSettings &ar_settings)
{
  gps_info = _gps_info;
  calculated_info = _calculated_info;
  airspace_renderer.SetSettings(ar_settings);
}

void
CrossSectionRenderer::Paint(Canvas &canvas, const PixelRect rc) const
{
  canvas.Clear(look.background_color);
  canvas.SetTextColor(look.text_color);
  canvas.Select(Fonts::map);

  ChartRenderer chart(chart_look, canvas, rc);

  if (!vec.IsValid() || !start.IsValid()) {
    chart.DrawNoData();
    return;
  }

  const fixed nav_altitude = gps_info.NavAltitudeAvailable()
    ? gps_info.nav_altitude
    : fixed_zero;
  fixed hmin = max(fixed_zero, nav_altitude - fixed(3300));
  fixed hmax = max(fixed(3300), nav_altitude + fixed(1000));

  chart.ResetScale();
  chart.ScaleXFromValue(fixed_zero);
  chart.ScaleXFromValue(vec.distance);
  chart.ScaleYFromValue(hmin);
  chart.ScaleYFromValue(hmax);

  short elevations[NUM_SLICES];
  UpdateTerrain(elevations);

  if (airspace_database)
    airspace_renderer.Draw(canvas, chart, *airspace_database, start, vec,
                           ToAircraftState(Basic(), Calculated()));
  terrain_renderer.Draw(canvas, chart, elevations);
  PaintGlide(chart);
  PaintAircraft(canvas, chart, rc);
  PaintGrid(canvas, chart);
}

void
CrossSectionRenderer::UpdateTerrain(short *elevations) const
{
  if (terrain == NULL) {
    const auto invalid = RasterBuffer::TERRAIN_INVALID;
    std::fill(elevations, elevations + NUM_SLICES, invalid);
    return;
  }

  const GeoPoint point_diff = vec.EndPoint(start) - start;

  RasterTerrain::Lease map(*terrain);
  for (unsigned i = 0; i < NUM_SLICES; ++i) {
    const fixed slice_distance_factor = fixed(i) / (NUM_SLICES - 1);
    const GeoPoint slice_point = start + point_diff * slice_distance_factor;

    elevations[i] = map->GetHeight(slice_point);
  }
}

void
CrossSectionRenderer::PaintGlide(ChartRenderer &chart) const
{
  if (gps_info.ground_speed_available && gps_info.ground_speed > fixed(10)) {
    fixed t = vec.distance / gps_info.ground_speed;
    chart.DrawLine(fixed_zero, gps_info.nav_altitude, vec.distance,
                   gps_info.nav_altitude + calculated_info.average * t,
                   ChartLook::STYLE_BLUETHIN);
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

  RasterPoint line[4];
  line[0] = chart.ToScreen(fixed_zero, gps_info.nav_altitude);
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
  canvas.SetTextColor(look.text_color);

  chart.DrawXGrid(Units::ToSysDistance(fixed(5)), fixed_zero,
                  look.grid_pen, fixed(5), true);
  chart.DrawYGrid(Units::ToSysAltitude(fixed(1000)), fixed_zero,
                  look.grid_pen, fixed(1000), true);

  chart.DrawXLabel(_T("D"), Units::GetDistanceName());
  chart.DrawYLabel(_T("h"), Units::GetAltitudeName());
}
