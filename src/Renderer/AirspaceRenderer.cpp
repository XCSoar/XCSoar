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

#include "AirspaceRenderer.hpp"
#include "ComputerSettings.hpp"
#include "MapSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Canvas.hpp"
#include "MapWindow/MapCanvas.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceVisitor.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "MapWindow/MapDrawHelper.hpp"
#include "Screen/Layout.hpp"
#include "NMEA/Aircraft.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

class AirspaceWarningCopy
{
private:
  StaticArray<const AbstractAirspace *,64> ids_inside, ids_warning, ids_acked;
  StaticArray<GeoPoint,32> locs;

public:
  void Visit(const AirspaceWarning& as) {
    if (as.GetWarningState() == AirspaceWarning::WARNING_INSIDE) {
      ids_inside.checked_append(&as.GetAirspace());
    } else if (as.GetWarningState() > AirspaceWarning::WARNING_CLEAR) {
      ids_warning.checked_append(&as.GetAirspace());
      locs.checked_append(as.GetSolution().location);
    }

    if (!as.IsAckExpired())
      ids_acked.checked_append(&as.GetAirspace());
  }

  void Visit(const AirspaceWarningManager &awm) {
    for (auto i = awm.begin(), end = awm.end(); i != end; ++i)
      Visit(*i);
  }

  void Visit(const ProtectedAirspaceWarningManager &awm) {
    const ProtectedAirspaceWarningManager::Lease lease(awm);
    Visit(lease);
  }

  const StaticArray<GeoPoint,32> &get_locations() const {
    return locs;
  }
  bool is_warning(const AbstractAirspace& as) const {
    return as.IsActive() && find(as, ids_warning);
  }
  bool is_acked(const AbstractAirspace& as) const {
    return (!as.IsActive()) || find(as, ids_acked);
  }
  bool is_inside(const AbstractAirspace& as) const {
    return as.IsActive() && find(as, ids_inside);
  }

  void visit_warned(AirspaceVisitor &visitor) const {
    for (auto it = ids_warning.begin(), end = ids_warning.end(); it != end; ++it)
      if (!is_acked(**it))
        visitor.Visit(**it);
  }

  void visit_inside(AirspaceVisitor &visitor) const {
    for (auto it = ids_inside.begin(), end = ids_inside.end(); it != end; ++it)
      if (!is_acked(**it))
        visitor.Visit(**it);
  }

private:
  bool find(const AbstractAirspace& as, 
            const StaticArray<const AbstractAirspace *,64> &list) const {
    return list.contains(&as);
  }
};


class AirspaceMapVisible: public AirspaceVisiblePredicate
{
private:
  const AirspaceWarningCopy& m_warnings;

public:
  AirspaceMapVisible(const AirspaceComputerSettings &_computer_settings,
                     const AirspaceRendererSettings &_renderer_settings,
                     const AircraftState& _state,
                     const AirspaceWarningCopy& warnings)
    :AirspaceVisiblePredicate(_computer_settings, _renderer_settings, _state),
     m_warnings(warnings) {}

  bool condition(const AbstractAirspace& airspace) const {
    return AirspaceVisiblePredicate::condition(airspace)
      || m_warnings.is_inside(airspace)
      || m_warnings.is_warning(airspace);
  }
};

#ifdef ENABLE_OPENGL

class AirspaceVisitorRenderer : public AirspaceVisitor, protected MapCanvas
{
  const AirspaceLook &airspace_look;
  const AirspaceWarningCopy& m_warnings;
  const AirspaceRendererSettings &settings;

public:
  AirspaceVisitorRenderer(Canvas &_canvas, const WindowProjection &_projection,
                          const AirspaceLook &_airspace_look,
                          const AirspaceWarningCopy& warnings,
                          const AirspaceRendererSettings &_settings)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(fixed(1.1))),
     airspace_look(_airspace_look),
     m_warnings(warnings),
     settings(_settings)
  {
    glStencilMask(0xff);
    glClear(GL_STENCIL_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  ~AirspaceVisitorRenderer() {
    glStencilMask(0xff);
  }

public:
  void Visit(const AirspaceCircle& airspace) {
    RasterPoint screen_center = projection.GeoToScreen(airspace.GetCenter());
    unsigned screen_radius = projection.GeoToScreenDistance(airspace.GetRadius());
    GLEnable stencil(GL_STENCIL_TEST);

    if (!m_warnings.is_acked(airspace)) {
      GLEnable blend(GL_BLEND);
      setup_interior(airspace);
      if (m_warnings.is_warning(airspace) ||
          m_warnings.is_inside(airspace) ||
          airspace_look.thick_pen.GetWidth() >= 2 * screen_radius) {
        // fill whole circle
        canvas.circle(screen_center.x, screen_center.y, screen_radius);
      } else {
        // draw a ring inside the circle
        Color color = settings.classes[airspace.GetType()].color;
        Pen pen_donut(airspace_look.thick_pen.GetWidth() / 2, color.WithAlpha(90));
        canvas.SelectHollowBrush();
        canvas.Select(pen_donut);
        canvas.circle(screen_center.x, screen_center.y,
                      screen_radius - airspace_look.thick_pen.GetWidth() / 4);
      }
    }

    // draw outline
    setup_outline(airspace);
    canvas.circle(screen_center.x, screen_center.y, screen_radius);
  }

  void Visit(const AirspacePolygon& airspace) {
    if (!prepare_polygon(airspace.GetPoints()))
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
      canvas.SelectBlackPen();
    else
      canvas.Select(airspace_look.pens[airspace.GetType()]);
    canvas.SelectHollowBrush();
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

    Color color = settings.classes[airspace.GetType()].color;
    canvas.Select(Brush(color.WithAlpha(90)));
    canvas.SelectNullPen();
  }

  void set_fillstencil() {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 3, 3);
    glStencilMask(1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    canvas.SelectHollowBrush();
    canvas.Select(airspace_look.thick_pen);
  }

  void clear_fillstencil() {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 3, 3);
    glStencilMask(1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    canvas.SelectHollowBrush();
    canvas.Select(airspace_look.thick_pen);
  }
};

class AirspaceFillRenderer : public AirspaceVisitor, protected MapCanvas
{
  const AirspaceLook &airspace_look;
  const AirspaceWarningCopy& m_warnings;
  const AirspaceRendererSettings &settings;

public:
  AirspaceFillRenderer(Canvas &_canvas, const WindowProjection &_projection,
                       const AirspaceLook &_airspace_look,
                       const AirspaceWarningCopy& warnings,
                       const AirspaceRendererSettings &_settings)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(fixed(1.1))),
     airspace_look(_airspace_look),
     m_warnings(warnings),
     settings(_settings)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

public:
  void Visit(const AirspaceCircle& airspace) {
    RasterPoint screen_center = projection.GeoToScreen(airspace.GetCenter());
    unsigned screen_radius = projection.GeoToScreenDistance(airspace.GetRadius());

    {
      GLEnable blend(GL_BLEND);
      setup_interior(airspace);
      canvas.circle(screen_center.x, screen_center.y, screen_radius);
    }

    // draw outline
    setup_outline(airspace);
    canvas.circle(screen_center.x, screen_center.y, screen_radius);
  }

  void Visit(const AirspacePolygon& airspace) {
    if (!prepare_polygon(airspace.GetPoints()))
      return;

    if (!m_warnings.is_acked(airspace)) {
      // fill interior without overpainting any previous outlines
      {
        setup_interior(airspace);
        GLEnable blend(GL_BLEND);
        draw_prepared();
      }
    }

    // draw outline
    setup_outline(airspace);
    draw_prepared();
  }

private:
  void setup_outline(const AbstractAirspace &airspace) {
    if (settings.black_outline)
      canvas.SelectBlackPen();
    else
      canvas.Select(airspace_look.pens[airspace.GetType()]);
    canvas.SelectHollowBrush();
  }

  void setup_interior(const AbstractAirspace &airspace) {
    Color color = settings.classes[airspace.GetType()].color;
    canvas.Select(Brush(color.WithAlpha(48)));
    canvas.SelectNullPen();
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
     m_warnings(warnings)
  {
    switch (settings.fill_mode) {
    case AirspaceRendererSettings::FillMode::DEFAULT:
      m_use_stencil = !IsAncientHardware();
      break;

    case AirspaceRendererSettings::FillMode::ALL:
      m_use_stencil = false;
      break;

    case AirspaceRendererSettings::FillMode::PADDING:
      m_use_stencil = true;
      break;
    }
  }

  void Visit(const AirspaceCircle& airspace) {
    if (m_warnings.is_acked(airspace))
      return;

    buffer_render_start();
    set_buffer_pens(airspace);

    RasterPoint center = m_proj.GeoToScreen(airspace.GetCenter());
    unsigned radius = m_proj.GeoToScreenDistance(airspace.GetRadius());
    draw_circle(center, radius);
  }

  void Visit(const AirspacePolygon& airspace) {
    if (m_warnings.is_acked(airspace))
      return;

    buffer_render_start();
    set_buffer_pens(airspace);
    draw_search_point_vector(airspace.GetPoints());
  }

  void draw_intercepts() {
    buffer_render_finish();
  }

private:
  void set_buffer_pens(const AbstractAirspace &airspace) {
    AirspaceClass airspace_class = airspace.GetType();

#ifndef HAVE_HATCHED_BRUSH
    m_buffer.Select(airspace_look.solid_brushes[airspace_class]);
#else /* HAVE_HATCHED_BRUSH */

#ifdef HAVE_ALPHA_BLEND
    if (settings.transparency && AlphaBlendAvailable()) {
      m_buffer.Select(airspace_look.solid_brushes[airspace_class]);
    } else {
#endif
      // this color is used as the black bit
      m_buffer.SetTextColor(LightColor(settings.classes[airspace_class].color));

      // get brush, can be solid or a 1bpp bitmap
      m_buffer.Select(airspace_look.brushes[settings.classes[airspace_class].brush]);

      m_buffer.SetBackgroundOpaque();
      m_buffer.SetBackgroundColor(COLOR_WHITE);
#ifdef HAVE_ALPHA_BLEND
    }
#endif

    m_buffer.SelectNullPen();

    if (m_warnings.is_warning(airspace) || m_warnings.is_inside(airspace)) {
      m_stencil.SelectBlackBrush();
      m_stencil.Select(airspace_look.medium_pen);
    } else {
      m_stencil.Select(airspace_look.thick_pen);
      m_stencil.SelectHollowBrush();
    }

#endif /* HAVE_HATCHED_BRUSH */
  }

  const AirspaceWarningCopy& m_warnings;
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
               _projection.GetScreenBounds().Scale(fixed(1.1))),
     airspace_look(_airspace_look),
     black(_black) {
    if (black)
      canvas.SelectBlackPen();
    canvas.SelectHollowBrush();
  }

protected:
  void setup_canvas(const AbstractAirspace &airspace) {
    if (!black)
      canvas.Select(airspace_look.pens[airspace.GetType()]);
  }

public:
  void Visit(const AirspaceCircle& airspace) {
    setup_canvas(airspace);
    circle(airspace.GetCenter(), airspace.GetRadius());
  }

  void Visit(const AirspacePolygon& airspace) {
    setup_canvas(airspace);
    draw(airspace.GetPoints());
  }
};

#endif // !ENABLE_OPENGL

void
AirspaceRenderer::DrawIntersections(Canvas &canvas,
                                    const WindowProjection &projection) const
{
  for (unsigned i = m_airspace_intersections.size(); i--;) {
    RasterPoint sc;
    if (projection.GeoToScreenIfVisible(m_airspace_intersections[i], sc))
      airspace_look.intercept_icon.Draw(canvas, sc.x, sc.y);
  }
}

void
AirspaceRenderer::Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
                       Canvas &buffer_canvas, Canvas &stencil_canvas,
#endif
                       const WindowProjection &projection,
                       const AirspaceRendererSettings &settings,
                       const AirspaceWarningCopy &awc,
                       const AirspacePredicate &visible)
{
  if (airspace_database == NULL)
    return;

#ifdef ENABLE_OPENGL
  if (settings.fill_mode == AirspaceRendererSettings::FillMode::ALL) {
    AirspaceFillRenderer renderer(canvas, projection, airspace_look, awc,
                                  settings);
    airspace_database->visit_within_range(projection.GetGeoScreenCenter(),
                                          projection.GetScreenDistanceMeters(),
                                          renderer, visible);
  } else {
    AirspaceVisitorRenderer renderer(canvas, projection, airspace_look, awc,
                                     settings);
    airspace_database->visit_within_range(projection.GetGeoScreenCenter(),
                                          projection.GetScreenDistanceMeters(),
                                          renderer, visible);
  }
#else
  MapDrawHelper helper(canvas, buffer_canvas, stencil_canvas, projection,
                       settings);
  AirspaceVisitorMap v(helper, awc, settings,
                       airspace_look);

  // JMW TODO wasteful to draw twice, can't it be drawn once?
  // we are using two draws so borders go on top of everything

  airspace_database->visit_within_range(projection.GetGeoScreenCenter(),
                                        projection.GetScreenDistanceMeters(),
                                        v, visible);

  awc.visit_warned(v);
  awc.visit_inside(v);

  v.draw_intercepts();

  AirspaceOutlineRenderer outline_renderer(canvas, projection,
                                           airspace_look,
                                           settings.black_outline);
  airspace_database->visit_within_range(projection.GetGeoScreenCenter(),
                                        projection.GetScreenDistanceMeters(),
                                        outline_renderer, visible);
  awc.visit_warned(outline_renderer);
  awc.visit_inside(outline_renderer);
#endif

  m_airspace_intersections = awc.get_locations();
}

void
AirspaceRenderer::Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
                       Canvas &buffer_canvas, Canvas &stencil_canvas,
#endif
                       const WindowProjection &projection,
                       const AirspaceRendererSettings &settings)
{
  if (airspace_database == NULL)
    return;

  AirspaceWarningCopy awc;
  if (airspace_warnings != NULL)
    awc.Visit(*airspace_warnings);

  Draw(canvas,
#ifndef ENABLE_OPENGL
       buffer_canvas, stencil_canvas,
#endif
       projection, settings, awc, AirspacePredicateTrue());
}

void
AirspaceRenderer::Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
                       Canvas &buffer_canvas, Canvas &stencil_canvas,
#endif
                       const WindowProjection &projection,
                       const MoreData &basic,
                       const DerivedInfo &calculated,
                       const AirspaceComputerSettings &computer_settings,
                       const AirspaceRendererSettings &settings)
{
  if (airspace_database == NULL)
    return;

  AirspaceWarningCopy awc;
  if (airspace_warnings != NULL)
    awc.Visit(*airspace_warnings);

  const AirspaceMapVisible visible(computer_settings, settings,
                                   ToAircraftState(basic, calculated), awc);
  Draw(canvas,
#ifndef ENABLE_OPENGL
       buffer_canvas, stencil_canvas,
#endif
       projection, settings, awc, visible);
}
