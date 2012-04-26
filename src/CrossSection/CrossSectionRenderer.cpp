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

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

/**
 * Local visitor class used for rendering airspaces in the CrossSectionRenderer
 */
class AirspaceIntersectionVisitorSlice: public AirspaceIntersectionVisitor
{
  /** Canvas to draw on */
  Canvas &canvas;
  /** ChartRenderer for scaling the airspace CrossSection */
  const ChartRenderer &chart;
  /** MapSettings for reading airspace colors, pen and brushes */
  const AirspaceRendererSettings &settings;

  const AirspaceLook &airspace_look;

  /** GeoPoint at the left of the CrossSection */
  const GeoPoint start;
  /** AltitudeState instance used for AGL-based airspaces */
  const AltitudeState& state;

public:
  /**
   * Constructor of the AirspaceIntersectionVisitorSlice class
   * @param _canvas The canvas to draw to
   * @param _chart ChartRenderer instance for scaling coordinates
   * @param _settings settings for colors, pens and brushes
   * @param _start GeoPoint at the left of the CrossSection
   * @param _state AltitudeState instance used for AGL-based airspaces
   */
  AirspaceIntersectionVisitorSlice(Canvas &_canvas,
                                   const ChartRenderer &_chart,
                                   const AirspaceRendererSettings &_settings,
                                   const AirspaceLook &_airspace_look,
                                   const GeoPoint _start,
                                   const AltitudeState& _state) :
    canvas(_canvas), chart(_chart), settings(_settings),
    airspace_look(_airspace_look),
    start(_start), state(_state) {}

  /**
   * Render an airspace box to the canvas
   * @param rc On-screen coordinates of the box
   * @param brush Brush to use
   * @param black Use black pen?
   * @param type Airspace class
   */
  void RenderBox(const PixelRect rc, const Brush &brush,
                 bool black, int type) const
  {
    // Enable "transparency" effect
#ifdef ENABLE_OPENGL
    GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#elif defined(USE_GDI)
    canvas.SetMixMask();
#endif /* GDI */

    // Use filling brush without outline
    canvas.Select(brush);
    canvas.SelectNullPen();

    // Draw thick brushed outlines
    PixelScalar border_width = Layout::Scale(10);
    if ((rc.right - rc.left) > border_width * 2 &&
        (rc.bottom - rc.top) > border_width * 2) {
      PixelRect border = rc;
      border.left += border_width;
      border.right -= border_width;
      border.top += border_width;
      border.bottom -= border_width;

      // Left border
      canvas.Rectangle(rc.left, rc.top, border.left, rc.bottom);

      // Right border
      canvas.Rectangle(border.right, rc.top, rc.right, rc.bottom);

      // Bottom border
      canvas.Rectangle(border.left, border.bottom, border.right, rc.bottom);

      // Top border
      canvas.Rectangle(border.left, rc.top, border.right, border.top);
    } else {
      // .. or fill the entire rect if the outlines would overlap
      canvas.Rectangle(rc.left, rc.top, rc.right, rc.bottom);
    }

    // Disable "transparency" effect
#ifdef ENABLE_OPENGL
    glDisable(GL_BLEND);
#elif defined(USE_GDI)
    canvas.SetMixCopy();
#endif /* GDI */

    // Use transparent brush and type-dependent pen for the outlines
    canvas.SelectHollowBrush();
    if (black)
      canvas.SelectBlackPen();
    else
      canvas.Select(airspace_look.pens[type]);

    // Draw thin outlines
    canvas.Rectangle(rc.left, rc.top, rc.right, rc.bottom);
  }

  /**
   * Renders the AbstractAirspace on the canvas
   * @param as AbstractAirspace to render
   */
  void
  Render(const AbstractAirspace& as) const
  {
    int type = as.GetType();
    if (type <= 0)
      return;

    // No intersections for this airspace
    if (intersections.empty())
      return;

    // Select pens and brushes
#ifndef USE_GDI
    Color color = settings.classes[type].color;
#ifdef ENABLE_OPENGL
    color = color.WithAlpha(48);
#endif
    Brush brush(color);
#else
    const Brush &brush = airspace_look.brushes[settings.classes[type].brush];
    canvas.SetTextColor(LightColor(settings.classes[type].color));
#endif

    PixelRect rcd;
    // Calculate top and bottom coordinate
    rcd.top = chart.ScreenY(as.GetTopAltitude(state));
    if (as.IsBaseTerrain())
      rcd.bottom = chart.ScreenY(fixed_zero);
    else
      rcd.bottom = chart.ScreenY(as.GetBaseAltitude(state));

    // Iterate through the intersections
    for (auto it = intersections.begin(); it != intersections.end(); ++it) {
      const GeoPoint &p_start = it->first;
      const GeoPoint &p_end = it->second;

      rcd.left = chart.ScreenX(start.Distance(p_start));

      // only one edge found, next edge must be beyond screen
      if (p_start == p_end)
        rcd.right = chart.ScreenX(chart.GetXMax());
      else
        rcd.right = chart.ScreenX(start.Distance(p_end));

      // Draw the airspace
      RenderBox(rcd, brush, settings.black_outline, type);
    }
  }

  /**
   * Visitor function for intersectingAirspaceCircle objects
   * @param as Intersecting AirspaceCircle instance
   */
  void
  Visit(const AirspaceCircle& as)
  {
    Render(as);
  }

  /**
   * Visitor function for intersecting AirspacePolygon objects
   * @param as Intersecting AirspacePolygon instance
   */
  void
  Visit(const AirspacePolygon& as)
  {
    Render(as);
  }
};

CrossSectionRenderer::CrossSectionRenderer(const CrossSectionLook &_look,
                                       const AirspaceLook &_airspace_look,
                                       const ChartLook &_chart_look)
  :look(_look), airspace_look(_airspace_look), chart_look(_chart_look),
  terrain_renderer(look), terrain(NULL), airspace_database(NULL),
  start(Angle::Zero(), Angle::Zero()),
   vec(fixed(50000), Angle::Zero()) {}

void
CrossSectionRenderer::ReadBlackboard(const MoreData &_gps_info,
                                   const DerivedInfo &_calculated_info,
                                   const AirspaceRendererSettings &_ar_settings)
{
  gps_info = _gps_info;
  calculated_info = _calculated_info;
  airspace_renderer_settings = _ar_settings;
}

void
CrossSectionRenderer::Paint(Canvas &canvas, const PixelRect rc) const
{
  canvas.clear(look.background_color);
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

  PaintAirspaces(canvas, chart);
  terrain_renderer.Draw(canvas, chart, elevations);
  PaintGlide(chart);
  PaintAircraft(canvas, chart, rc);
  PaintGrid(canvas, chart);
}

void
CrossSectionRenderer::PaintAirspaces(Canvas &canvas,
                                   const ChartRenderer &chart) const
{
  // Quit early if no airspace database available
  if (airspace_database == NULL)
    return;

  // Create IntersectionVisitor to render to the canvas
  AirspaceIntersectionVisitorSlice ivisitor(canvas, chart,
                                            airspace_renderer_settings,
                                            airspace_look,
                                            start,
                                            ToAircraftState(Basic(),
                                                            Calculated()));

  // Call visitor with intersecting airspaces
  airspace_database->VisitIntersecting(start, vec.EndPoint(start), ivisitor);
}

void
CrossSectionRenderer::UpdateTerrain(short *elevations) const
{
  if (terrain == NULL)
    return;

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
