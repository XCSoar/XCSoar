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

#include "AirspaceXSRenderer.hpp"
#include "Renderer/ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Navigation/Aircraft.hpp"
#include "Geo/GeoVector.hpp"
#include "Util/StringCompare.hxx"

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
  void RenderBox(const PixelRect rc, AirspaceClass type) const;

  /**
   * Renders the AbstractAirspace on the canvas
   * @param as AbstractAirspace to render
   */
  void Render(const AbstractAirspace &as) const;

  virtual void Visit(const AbstractAirspace &as) override {
    Render(as);
  }
};

inline void
AirspaceIntersectionVisitorSlice::RenderBox(const PixelRect rc,
                                            AirspaceClass type) const
{
  if (AirspacePreviewRenderer::PrepareFill(canvas, type, airspace_look,
                                           settings)) {
    const auto &class_settings = settings.classes[type];

    // Draw thick brushed outlines
    const unsigned border_width = class_settings.fill_mode ==
      AirspaceClassRendererSettings::FillMode::PADDING
      ? Layout::ScalePenWidth(10)
      : 0;

    if (border_width > 0 &&
        rc.GetWidth() > border_width * 2 &&
        rc.GetHeight() > border_width * 2) {
      PixelRect border = rc;
      border.Grow(-(int)border_width);

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
  if (AirspacePreviewRenderer::PrepareOutline(canvas, type, airspace_look,
                                              settings))
    canvas.Rectangle(rc.left, rc.top, rc.right, rc.bottom);
}

inline void
AirspaceIntersectionVisitorSlice::Render(const AbstractAirspace &as) const
{
  AirspaceClass type = as.GetType();

  // No intersections for this airspace
  if (intersections.empty())
    return;

  if (!IsAirspaceTypeVisible(as, settings))
    return;

  PixelRect rcd;
  // Calculate top and bottom coordinate
  rcd.top = chart.ScreenY(as.GetTopAltitude(state));
  if (as.IsBaseTerrain())
    rcd.bottom = chart.ScreenY(0);
  else
    rcd.bottom = chart.ScreenY(as.GetBaseAltitude(state));

  int min_x = 1024, max_x = 0;

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

    if (rcd.left < min_x)
      min_x = rcd.left;

    if (rcd.right > max_x)
      max_x = rcd.right;

    // Draw the airspace
    RenderBox(rcd, type);
  }

  min_x += Layout::GetTextPadding();
  max_x -= Layout::GetTextPadding();

  /* draw the airspace name */
  const TCHAR *name = as.GetName();
  if (name != nullptr && !StringIsEmpty(name) && min_x < max_x) {
    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(COLOR_BLACK);

    const unsigned max_width = max_x - min_x;

    const PixelSize name_size = canvas.CalcTextSize(name);
    const int x = unsigned(name_size.cx) >= max_width
      ? min_x
      : (min_x + max_x - name_size.cx) / 2;
    const int y = (rcd.top + rcd.bottom - name_size.cy) / 2;

    canvas.DrawClippedText(x, y, max_x - x, name);
  }
}


void
AirspaceXSRenderer::Draw(Canvas &canvas, const ChartRenderer &chart,
                         const Airspaces &database, const GeoPoint &start,
                         const GeoVector &vec, const AircraftState &state) const
{
  canvas.Select(*look.name_font);

  // Create IntersectionVisitor to render to the canvas
  AirspaceIntersectionVisitorSlice ivisitor(
      canvas, chart, settings, look, start, state);

  // Call visitor with intersecting airspaces
  database.VisitIntersecting(start, vec.EndPoint(start), true, ivisitor);
}
