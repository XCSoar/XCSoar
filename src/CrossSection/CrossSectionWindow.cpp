/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "CrossSectionWindow.hpp"
#include "Components.hpp"
#include "GlideComputer.hpp"
#include "Interface.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Chart.hpp"
#include "Screen/Graphics.hpp"
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Units.hpp"

#define AIRSPACE_SCANSIZE_X 16

class AirspaceIntersectionVisitorSlice: public AirspaceIntersectionVisitor
{
public:
  AirspaceIntersectionVisitorSlice(Canvas &canvas, Chart &chart,
                                   const SETTINGS_MAP &settings,
                                   const GeoPoint start,
                                   const ALTITUDE_STATE& state) :
    m_canvas(canvas), m_chart(chart), m_settings(settings), m_start(start), m_state(state)
  {
  }

  void
  Render(const AbstractAirspace& as)
  {
    int type = as.get_type();

    if (m_intersections.empty())
      return;

    if (m_settings.bAirspaceBlackOutline)
      m_canvas.black_pen();
    else
      m_canvas.select(Graphics::hAirspacePens[type]);

    m_canvas.select(Graphics::GetAirspaceBrushByClass(type, m_settings));
    m_canvas.set_text_color(Graphics::GetAirspaceColourByClass(type, m_settings));

    RECT rcd;
    rcd.top = m_chart.screenY(as.get_top_altitude(m_state));
    if (as.is_base_terrain())
      rcd.bottom = m_chart.screenY(fixed_zero);
    else
      rcd.bottom = m_chart.screenY(as.get_base_altitude(m_state));

    for (AirspaceIntersectionVector::const_iterator it = m_intersections.begin();
         it != m_intersections.end(); ++it) {
      const GeoPoint p_start = it->first;
      const GeoPoint p_end = it->second;
      const fixed distance_start = m_start.distance(p_start);
      const fixed distance_end = m_start.distance(p_end);

      rcd.left = m_chart.screenX(distance_start);
      rcd.right = m_chart.screenX(distance_end);

      // only one edge found, next edge must be beyond screen
      if ((rcd.left == rcd.right) && (p_start == p_end)) {
        rcd.right = m_chart.screenX(m_chart.getXmax());
      }

      m_canvas.rectangle(rcd.left, rcd.top, rcd.right, rcd.bottom);
    }
  }

  void
  render(const AbstractAirspace& as)
  {
    if (as.get_type() >= 0)
      Render(as);
  }

  void
  Visit(const AirspaceCircle& as)
  {
    render(as);
  }

  void
  Visit(const AirspacePolygon& as)
  {
    render(as);
  }

private:
  Canvas& m_canvas;
  Chart& m_chart;
  const SETTINGS_MAP& m_settings;
  const GeoPoint m_start;
  const ALTITUDE_STATE& m_state;
};

CrossSectionWindow::CrossSectionWindow() :
  terrain(NULL), airspace_database(NULL),
  start(Angle::native(fixed_zero), Angle::native(fixed_zero)),
  vec(fixed(50000), Angle::native(fixed_zero)) {}

void
CrossSectionWindow::ReadBlackboard(const NMEA_INFO &_gps_info,
                                   const DERIVED_INFO &_calculated_info,
                                   const SETTINGS_MAP &_settings_map)
{
  gps_info = _gps_info;
  calculated_info = _calculated_info;
  settings_map = _settings_map;
}

void
CrossSectionWindow::Paint(Canvas &canvas, const RECT rc)
{
  fixed hmin = max(fixed_zero, gps_info.GPSAltitude - fixed(3300));
  fixed hmax = max(fixed(3300), gps_info.GPSAltitude + fixed(1000));

  Chart chart(canvas, rc);
  chart.ResetScale();
  chart.ScaleXFromValue(fixed_zero);
  chart.ScaleXFromValue(vec.Distance);
  chart.ScaleYFromValue(hmin);
  chart.ScaleYFromValue(hmax);

  PaintAirspaces(canvas, chart);
  PaintTerrain(canvas, chart);
  PaintGlide(chart);
  PaintAircraft(canvas, chart, rc);
  PaintGrid(canvas, chart);
}

void
CrossSectionWindow::PaintAirspaces(Canvas &canvas, Chart &chart)
{
  if (airspace_database == NULL)
    return;

  AirspaceIntersectionVisitorSlice ivisitor(canvas, chart, settings_map,
                                            start, ToAircraftState(gps_info));
  airspace_database->visit_intersecting(start, vec, ivisitor);
}

void
CrossSectionWindow::PaintTerrain(Canvas &canvas, Chart &chart)
{
  if (terrain == NULL)
    return;

  const GeoPoint p_diff = vec.end_point(start) - start;

  RasterTerrain::Lease map(*terrain);

  RasterPoint points[2 + AIRSPACE_SCANSIZE_X];

  points[0].x = chart.screenX(vec.Distance);
  points[0].y = chart.screenY(fixed_zero);
  points[1].x = chart.screenX(fixed_zero);
  points[1].y = chart.screenY(fixed_zero);

  unsigned i = 2;
  for (unsigned j = 0; j < AIRSPACE_SCANSIZE_X; ++j) {
    const fixed t_this = fixed(j) / (AIRSPACE_SCANSIZE_X - 1);
    const GeoPoint p_this = start + p_diff * t_this;

    short h = map->GetField(p_this);
    if (RasterBuffer::is_special(h)) {
      if (RasterBuffer::is_water(h))
        /* water is at 0m MSL */
        /* XXX paint in blue? */
        h = 0;
      else
        /* skip "unknown" values */
        continue;
    }

    RasterPoint p;
    p.x = chart.screenX(t_this * vec.Distance);
    p.y = chart.screenY(fixed(h));

    points[i++] = p;
  }

  if (i >= 4) {
    canvas.null_pen();
    canvas.select(bTerrain);
    canvas.polygon(&points[0], i);
  }
}

void
CrossSectionWindow::PaintGlide(Chart &chart)
{
  if (gps_info.GroundSpeed > fixed(10)) {
    fixed t = vec.Distance / gps_info.GroundSpeed;
    chart.DrawLine(fixed_zero, gps_info.GPSAltitude, vec.Distance,
                   gps_info.GPSAltitude + calculated_info.Average30s * t,
                   Chart::STYLE_BLUETHIN);
  }
}

void
CrossSectionWindow::PaintAircraft(Canvas &canvas, const Chart &chart,
                                  const RECT rc)
{
  canvas.white_pen();
  canvas.white_brush();

  RasterPoint line[4];
  line[0].x = chart.screenX(fixed_zero);
  line[0].y = chart.screenY(gps_info.GPSAltitude);
  line[1].x = rc.left;
  line[1].y = line[0].y;
  line[2].x = line[1].x;
  line[2].y = line[0].y - (line[0].x - line[1].x) / 2;
  line[3].x = (line[1].x + line[0].x) / 2;
  line[3].y = line[0].y;
  canvas.polygon(line, 4);
}

void
CrossSectionWindow::PaintGrid(Canvas &canvas, Chart &chart)
{
  canvas.set_text_color(Color(0xff, 0xff, 0xff));

  chart.DrawXGrid(Units::ToSysDistance(fixed(5)), fixed_zero,
                  Chart::STYLE_THINDASHPAPER, fixed(5), true);
  chart.DrawYGrid(Units::ToSysAltitude(fixed(1000)), fixed_zero,
                  Chart::STYLE_THINDASHPAPER, fixed(1000), true);

  chart.DrawXLabel(_T("D"));
  chart.DrawYLabel(_T("h"));
}

bool
CrossSectionWindow::on_create() {
  PaintWindow::on_create();

  bTerrain.set(Chart::GROUND_COLOUR);

  return true;
}

void
CrossSectionWindow::on_paint(Canvas &canvas)
{
  canvas.clear(Color(0x40, 0x40, 0x00));
  canvas.set_text_color(Color::WHITE);
  canvas.select(Fonts::Map);

  const RECT rc = get_client_rect();

  Paint(canvas, rc);
}
