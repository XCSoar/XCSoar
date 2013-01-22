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
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Navigation/Aircraft.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

/**
 * Local visitor class used for rendering airspaces in the CrossSectionRenderer
 */
class AirspaceIntersectionVisitorSlice final
  : public AirspaceIntersectionVisitor
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
  void
  RenderBox(const PixelRect rc, AirspaceClass type) const
  {
    if (AirspacePreviewRenderer::PrepareFill(
        canvas, type, airspace_look, settings)) {

      // Draw thick brushed outlines
      PixelScalar border_width = Layout::Scale(10);
      if ((rc.right - rc.left) > border_width * 2 &&
          (rc.bottom - rc.top) > border_width * 2 &&
          settings.classes[type].fill_mode ==
          AirspaceClassRendererSettings::FillMode::PADDING) {
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

      AirspacePreviewRenderer::UnprepareFill(canvas);
    }

    // Use transparent brush and type-dependent pen for the outlines
    if (AirspacePreviewRenderer::PrepareOutline(
        canvas, type, airspace_look, settings))
      canvas.Rectangle(rc.left, rc.top, rc.right, rc.bottom);
  }

  /**
   * Renders the AbstractAirspace on the canvas
   * @param as AbstractAirspace to render
   */
  void Render(const AbstractAirspace &as) const {
    AirspaceClass type = as.GetType();
    if (type <= 0)
      return;

    // No intersections for this airspace
    if (intersections.empty())
      return;

    PixelRect rcd;
    // Calculate top and bottom coordinate
    rcd.top = chart.ScreenY(as.GetTopAltitude(state));
    if (as.IsBaseTerrain())
      rcd.bottom = chart.ScreenY(fixed(0));
    else
      rcd.bottom = chart.ScreenY(as.GetBaseAltitude(state));

    // Iterate through the intersections
    for (const auto &i : intersections) {
      const GeoPoint &p_start = i.first;
      const GeoPoint &p_end = i.second;

      rcd.left = chart.ScreenX(start.Distance(p_start));

      // only one edge found, next edge must be beyond screen
      if (p_start == p_end)
        rcd.right = chart.ScreenX(chart.GetXMax());
      else
        rcd.right = chart.ScreenX(start.Distance(p_end));

      // Draw the airspace
      RenderBox(rcd, type);
    }
  }

  virtual void Visit(const AbstractAirspace &as) override {
    Render(as);
  }
};

void
AirspaceXSRenderer::Draw(Canvas &canvas, const ChartRenderer &chart,
                         const Airspaces &database, const GeoPoint &start,
                         const GeoVector &vec, const AircraftState &state) const
{
  // Create IntersectionVisitor to render to the canvas
  AirspaceIntersectionVisitorSlice ivisitor(
      canvas, chart, settings, look, start, state);

  // Call visitor with intersecting airspaces
  database.VisitIntersecting(start, vec.EndPoint(start), ivisitor);
}
