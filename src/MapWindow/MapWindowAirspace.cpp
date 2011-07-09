/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "MapWindow.hpp"
#include "MapCanvas.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceVisitor.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/AirspaceWarningVisitor.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "MapDrawHelper.hpp"
#include "Screen/Layout.hpp"
#include "NMEA/Aircraft.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

class AirspaceWarningCopy: 
  public AirspaceWarningVisitor
{
private:
  StaticArray<const AbstractAirspace *,64> ids_inside, ids_warning, ids_acked;
  StaticArray<GeoPoint,32> locs;

public:
  void Visit(const AirspaceWarning& as) {
    if (as.get_warning_state() == AirspaceWarning::WARNING_INSIDE) {
      ids_inside.checked_append(&as.get_airspace());
    } else if (as.get_warning_state() > AirspaceWarning::WARNING_CLEAR) {
      ids_warning.checked_append(&as.get_airspace());
      locs.checked_append(as.get_solution().location);
    }

    if (!as.get_ack_expired())
      ids_acked.checked_append(&as.get_airspace());
  }

  const StaticArray<GeoPoint,32> &get_locations() const {
    return locs;
  }
  bool is_warning(const AbstractAirspace& as) const {
    return as.is_active() && find(as, ids_warning);
  }
  bool is_acked(const AbstractAirspace& as) const {
    return (!as.is_active()) || find(as, ids_acked);
  }
  bool is_inside(const AbstractAirspace& as) const {
    return as.is_active() && find(as, ids_inside);
  }

  void visit_warned(AirspaceVisitor &visitor) {
    for (unsigned i = 0; i < ids_warning.size(); i++)
      if (!is_acked(*ids_warning[i]))
        visitor.Visit(*ids_warning[i]);
  }

  void visit_inside(AirspaceVisitor &visitor) {
    for (unsigned i = 0; i < ids_inside.size(); i++)
      if (!is_acked(*ids_inside[i]))
        visitor.Visit(*ids_inside[i]);
  }

private:
  bool find(const AbstractAirspace& as, 
            const StaticArray<const AbstractAirspace *,64> &list) const {
    return list.contains(&as);
  }
};


class AirspaceMapVisible: public AirspaceVisible
{
private:
  const bool &m_border;
  const AirspaceWarningCopy& m_warnings;

public:
  AirspaceMapVisible(const AirspaceComputerSettings &_computer_settings,
                     const AirspaceRendererSettings &_renderer_settings,
                     const AIRCRAFT_STATE& _state, const bool& _border,
                     const AirspaceWarningCopy& warnings)
    :AirspaceVisible(_computer_settings, _renderer_settings, _state),
     m_border(_border),
     m_warnings(warnings) {}

  virtual bool operator()(const AbstractAirspace& airspace) const {
    return condition(airspace);
  }

  bool condition(const AbstractAirspace& airspace) const {
    return parent_condition(airspace) 
      || m_warnings.is_inside(airspace)
      || m_warnings.is_warning(airspace);
  }
};

#ifdef ENABLE_OPENGL

class AirspaceRenderer : public AirspaceVisitor, protected MapCanvas
{
  const AirspaceLook &airspace_look;
  const AirspaceWarningCopy& m_warnings;
  const AirspaceRendererSettings &settings;
  Pen pen_thick;

public:
  AirspaceRenderer(Canvas &_canvas, const WindowProjection &_projection,
                   const AirspaceLook &_airspace_look,
                   const AirspaceWarningCopy& warnings,
                   const AirspaceRendererSettings &_settings)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().scale(fixed(1.1))),
     airspace_look(_airspace_look),
     m_warnings(warnings),
     settings(_settings),
     pen_thick(IBLSCALE(10), Color(0x00, 0x00, 0x00))
  {
    glStencilMask(0xff);
    glClear(GL_STENCIL_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  ~AirspaceRenderer() {
    glStencilMask(0xff);
  }

public:
  void Visit(const AirspaceCircle& airspace) {
    RasterPoint screen_center = projection.GeoToScreen(airspace.get_center());
    unsigned screen_radius = projection.GeoToScreenDistance(airspace.get_radius());
    GLEnable stencil(GL_STENCIL_TEST);

    {
      GLEnable blend(GL_BLEND);
      setup_interior(airspace);
      if (m_warnings.is_warning(airspace) ||
          m_warnings.is_inside(airspace) ||
          pen_thick.get_width() >= 2*screen_radius) {
        // fill whole circle
        canvas.circle(screen_center.x, screen_center.y, screen_radius);
      } else {
        // draw a ring inside the circle
        Color color = airspace_look.colors[settings.colours[airspace.get_type()]];
        Pen pen_donut(pen_thick.get_width()/2, color.with_alpha(90));
        canvas.hollow_brush();
        canvas.select(pen_donut);
        canvas.circle(screen_center.x, screen_center.y,
                      screen_radius - pen_thick.get_width()/4);
      }
    }

    // draw outline
    setup_outline(airspace);
    canvas.circle(screen_center.x, screen_center.y, screen_radius);
  }

  void Visit(const AirspacePolygon& airspace) {
    if (!prepare_polygon(airspace.get_points()))
      return;

    bool fill_airspace = m_warnings.is_warning(airspace) ||
                         m_warnings.is_inside(airspace);
    GLEnable stencil(GL_STENCIL_TEST);

    if (!m_warnings.is_acked(airspace)) {
      if (!fill_airspace) {
        // set stencil for filling (bit 0)
        set_fillstencil();
        draw_prepared();
      }

      // fill interior without overpainting any previous outlines
      {
        setup_interior(airspace, !fill_airspace);
        GLEnable blend(GL_BLEND);
        draw_prepared();
      }

      if (!fill_airspace) {
        // clear fill stencil (bit 0)
        clear_fillstencil();
        draw_prepared();
      }
    }

    // draw outline
    setup_outline(airspace);
    draw_prepared();
  }

private:
  void setup_outline(const AbstractAirspace &airspace) {
    // set bit 1 in stencil buffer, where an outline is drawn
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_ALWAYS, 3, 3);
    glStencilMask(2);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    if (settings.black_outline)
      canvas.black_pen();
    else
      canvas.select(airspace_look.pens[airspace.get_type()]);
    canvas.hollow_brush();
  }

  void setup_interior(const AbstractAirspace &airspace,
                      bool check_fillstencil = false) {
    // restrict drawing area and don't paint over previously drawn outlines
    if (check_fillstencil)
      glStencilFunc(GL_EQUAL, 1, 3);
    else
      glStencilFunc(GL_EQUAL, 0, 2);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    Color color = airspace_look.colors[settings.colours[airspace.get_type()]];
    canvas.select(Brush(color.with_alpha(90)));
    canvas.null_pen();
  }

  void set_fillstencil() {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 3, 3);
    glStencilMask(1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    canvas.hollow_brush();
    canvas.select(pen_thick);
  }

  void clear_fillstencil() {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 3, 3);
    glStencilMask(1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    canvas.hollow_brush();
    canvas.select(pen_thick);
  }
};

#else // !ENABLE_OPENGL

/**
 * Class to render airspaces onto map in two passes,
 * one for border, one for area.
 * This is a bit slow because projections are performed twice.
 * The old way of doing it was possibly faster but required a lot
 * of code overhead.
 */
class AirspaceVisitorMap: 
  public AirspaceVisitor,
  public MapDrawHelper
{
  const AirspaceLook &airspace_look;

public:
  AirspaceVisitorMap(MapDrawHelper &_helper,
                     const AirspaceWarningCopy& warnings,
                     const AirspaceRendererSettings &_settings,
                     const AirspaceLook &_airspace_look)
    :MapDrawHelper(_helper),
     airspace_look(_airspace_look),
     m_warnings(warnings),
     pen_thick(Pen::SOLID, IBLSCALE(10), Color(0x00, 0x00, 0x00)),
     pen_medium(Pen::SOLID, IBLSCALE(3), Color(0x00, 0x00, 0x00))
  {
    switch (settings.fill_mode) {
    case AirspaceRendererSettings::AS_FILL_DEFAULT:
      m_use_stencil = !is_ancient_hardware();
      break;

    case AirspaceRendererSettings::AS_FILL_ALL:
      m_use_stencil = false;
      break;

    case AirspaceRendererSettings::AS_FILL_PADDING:
      m_use_stencil = true;
      break;
    }
  }

  void Visit(const AirspaceCircle& airspace) {
    if (m_warnings.is_acked(airspace))
      return;

    buffer_render_start();
    set_buffer_pens(airspace);

    RasterPoint center = m_proj.GeoToScreen(airspace.get_center());
    unsigned radius = m_proj.GeoToScreenDistance(airspace.get_radius());
    draw_circle(m_buffer, center, radius);
  }

  void Visit(const AirspacePolygon& airspace) {
    if (m_warnings.is_acked(airspace))
      return;

    buffer_render_start();
    set_buffer_pens(airspace);
    draw_search_point_vector(m_buffer, airspace.get_points());
  }

  void draw_intercepts() {
    buffer_render_finish();
  }

private:
  void set_buffer_pens(const AbstractAirspace &airspace) {
    const unsigned color_index = settings.colours[airspace.get_type()];

#ifdef ENABLE_SDL
    m_buffer.select(airspace_look.solid_brushes[color_index]);
#else /* !SDL */

#ifdef HAVE_ALPHA_BLEND
    if (settings.transparency && AlphaBlendAvailable()) {
      m_buffer.select(airspace_look.solid_brushes[color_index]);
    } else {
#endif
      // this color is used as the black bit
      m_buffer.set_text_color(light_color(airspace_look.colors[color_index]));

      // get brush, can be solid or a 1bpp bitmap
      m_buffer.select(airspace_look.brushes[settings.brushes[airspace.get_type()]]);

      m_buffer.background_opaque();
      m_buffer.set_background_color(COLOR_WHITE);
#ifdef HAVE_ALPHA_BLEND
    }
#endif

    m_buffer.null_pen();

    if (m_warnings.is_warning(airspace) || m_warnings.is_inside(airspace)) {
      m_stencil.black_brush();
      m_stencil.select(pen_medium);
    } else {
      m_stencil.select(pen_thick);
      m_stencil.hollow_brush();
    }
#endif

  }

  const AirspaceWarningCopy& m_warnings;
  Pen pen_thick;
  Pen pen_medium;
};

class AirspaceOutlineRenderer
  :public AirspaceVisitor,
   protected MapCanvas
{
  const AirspaceLook &airspace_look;
  bool black;

public:
  AirspaceOutlineRenderer(Canvas &_canvas, const WindowProjection &_projection,
                          const AirspaceLook &_airspace_look,
                          bool _black)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().scale(fixed(1.1))),
     airspace_look(_airspace_look),
     black(_black) {
    if (black)
      canvas.black_pen();
    canvas.hollow_brush();
  }

protected:
  void setup_canvas(const AbstractAirspace &airspace) {
    if (!black)
      canvas.select(airspace_look.pens[airspace.get_type()]);
  }

public:
  void Visit(const AirspaceCircle& airspace) {
    setup_canvas(airspace);
    circle(airspace.get_center(), airspace.get_radius());
  }

  void Visit(const AirspacePolygon& airspace) {
    setup_canvas(airspace);
    draw(airspace.get_points());
  }
};

#endif // !ENABLE_OPENGL

void
MapWindow::DrawAirspaceIntersections(Canvas &canvas) const
{
  for (unsigned i = m_airspace_intersections.size(); i--;) {
    RasterPoint sc;
    if (render_projection.GeoToScreenIfVisible(m_airspace_intersections[i], sc))
      airspace_look.intercept_icon.draw(canvas, sc.x, sc.y);
  }
}

/**
 * Draws the airspace to the given canvas
 * @param canvas The drawing canvas
 * @param buffer The drawing buffer
 */
void
MapWindow::DrawAirspace(Canvas &canvas)
{
  if (airspace_database == NULL)
    return;

  AirspaceWarningCopy awc;
  if (airspace_warnings != NULL)
    airspace_warnings->visit_warnings(awc);

  const AirspaceMapVisible visible(SettingsComputer().airspace,
                                   SettingsMap().airspace,
                                   ToAircraftState(Basic(), Calculated()),
                                   false, awc);

#ifdef ENABLE_OPENGL
  AirspaceRenderer renderer(canvas, render_projection, airspace_look, awc,
                            SettingsMap().airspace);
  airspace_database->visit_within_range(render_projection.GetGeoScreenCenter(),
                                        render_projection.GetScreenDistanceMeters(),
                                        renderer, visible);
#else
  MapDrawHelper helper(canvas, buffer_canvas, stencil_canvas, render_projection,
                       SettingsMap().airspace);
  AirspaceVisitorMap v(helper, awc, SettingsMap().airspace,
                       airspace_look);

  // JMW TODO wasteful to draw twice, can't it be drawn once?
  // we are using two draws so borders go on top of everything

  airspace_database->visit_within_range(render_projection.GetGeoScreenCenter(),
                                        render_projection.GetScreenDistanceMeters(),
                                        v, visible);

  awc.visit_warned(v);
  awc.visit_inside(v);

  v.draw_intercepts();

  AirspaceOutlineRenderer outline_renderer(canvas, render_projection,
                                           airspace_look,
                                           SettingsMap().airspace.black_outline);
  airspace_database->visit_within_range(render_projection.GetGeoScreenCenter(),
                                        render_projection.GetScreenDistanceMeters(),
                                        outline_renderer, visible);
  awc.visit_warned(outline_renderer);
  awc.visit_inside(outline_renderer);
#endif

  m_airspace_intersections = awc.get_locations();
}
