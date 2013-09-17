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

#include "AirspaceRenderer.hpp"
#include "AirspaceRendererSettings.hpp"
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
#include "MapWindow/StencilMapCanvas.hpp"
#include "Screen/Layout.hpp"
#include "NMEA/Aircraft.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

#ifdef USE_GDI
#include "Screen/GDI/AlphaBlend.hpp"
#endif

class AirspaceWarningCopy
{
private:
  StaticArray<const AbstractAirspace *,64> ids_inside, ids_warning, ids_acked;
  StaticArray<GeoPoint,32> locations;

  unsigned serial;

public:
  unsigned GetSerial() const {
    return serial;
  }

  void Visit(const AirspaceWarning& as) {
    if (as.GetWarningState() == AirspaceWarning::WARNING_INSIDE) {
      ids_inside.checked_append(&as.GetAirspace());
    } else if (as.GetWarningState() > AirspaceWarning::WARNING_CLEAR) {
      ids_warning.checked_append(&as.GetAirspace());
      locations.checked_append(as.GetSolution().location);
    }

    if (!as.IsAckExpired())
      ids_acked.checked_append(&as.GetAirspace());
  }

  void Visit(const AirspaceWarningManager &awm) {
    serial = awm.GetSerial();

    for (auto i = awm.begin(), end = awm.end(); i != end; ++i)
      Visit(*i);
  }

  void Visit(const ProtectedAirspaceWarningManager &awm) {
    const ProtectedAirspaceWarningManager::Lease lease(awm);
    Visit(lease);
  }

  const StaticArray<GeoPoint,32> &GetLocations() const {
    return locations;
  }

  bool HasWarning(const AbstractAirspace &as) const {
    return as.IsActive() && Find(as, ids_warning);
  }

  bool IsAcked(const AbstractAirspace &as) const {
    return (!as.IsActive()) || Find(as, ids_acked);
  }

  bool IsInside(const AbstractAirspace &as) const {
    return as.IsActive() && Find(as, ids_inside);
  }

private:
  bool Find(const AbstractAirspace& as,
            const StaticArray<const AbstractAirspace *,64> &list) const {
    return list.contains(&as);
  }
};


class AirspaceMapVisible : public AirspacePredicate
{
  const AirspaceVisibility visible_predicate;
  const AirspaceWarningCopy &warnings;

public:
  AirspaceMapVisible(const AirspaceComputerSettings &_computer_settings,
                     const AirspaceRendererSettings &_renderer_settings,
                     const AircraftState& _state,
                     const AirspaceWarningCopy& _warnings)
    :visible_predicate(_computer_settings, _renderer_settings, _state),
     warnings(_warnings) {}

  bool operator()(const AbstractAirspace& airspace) const {
    return visible_predicate(airspace) ||
      warnings.IsInside(airspace) ||
      warnings.HasWarning(airspace);
  }
};

#ifdef ENABLE_OPENGL

class AirspaceVisitorRenderer final
  : public AirspaceVisitor, protected MapCanvas
{
  const AirspaceLook &look;
  const AirspaceWarningCopy &warning_manager;
  const AirspaceRendererSettings &settings;

public:
  AirspaceVisitorRenderer(Canvas &_canvas, const WindowProjection &_projection,
                          const AirspaceLook &_look,
                          const AirspaceWarningCopy &_warnings,
                          const AirspaceRendererSettings &_settings)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(fixed(1.1))),
     look(_look), warning_manager(_warnings), settings(_settings)
  {
    glStencilMask(0xff);
    glClear(GL_STENCIL_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  ~AirspaceVisitorRenderer() {
    glStencilMask(0xff);
  }

private:
  void VisitCircle(const AirspaceCircle &airspace) {
    const AirspaceClassRendererSettings &class_settings =
      settings.classes[airspace.GetType()];
    const AirspaceClassLook &class_look = look.classes[airspace.GetType()];

    RasterPoint screen_center = projection.GeoToScreen(airspace.GetCenter());
    unsigned screen_radius = projection.GeoToScreenDistance(airspace.GetRadius());

    if (!warning_manager.IsAcked(airspace) &&
        class_settings.fill_mode !=
        AirspaceClassRendererSettings::FillMode::NONE) {
      GLEnable stencil(GL_STENCIL_TEST);
      GLEnable blend(GL_BLEND);
      SetupInterior(airspace);
      if (warning_manager.HasWarning(airspace) ||
          warning_manager.IsInside(airspace) ||
          look.thick_pen.GetWidth() >= 2 * screen_radius ||
          class_settings.fill_mode ==
          AirspaceClassRendererSettings::FillMode::ALL) {
        // fill whole circle
        canvas.DrawCircle(screen_center.x, screen_center.y, screen_radius);
      } else {
        // draw a ring inside the circle
        Color color = class_look.fill_color;
        Pen pen_donut(look.thick_pen.GetWidth() / 2, color.WithAlpha(90));
        canvas.SelectHollowBrush();
        canvas.Select(pen_donut);
        canvas.DrawCircle(screen_center.x, screen_center.y,
                          screen_radius - look.thick_pen.GetWidth() / 4);
      }
    }

    // draw outline
    if (SetupOutline(airspace))
      canvas.DrawCircle(screen_center.x, screen_center.y, screen_radius);
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
    if (!PreparePolygon(airspace.GetPoints()))
      return;

    const AirspaceClassRendererSettings &class_settings =
      settings.classes[airspace.GetType()];

    bool fill_airspace = warning_manager.HasWarning(airspace) ||
      warning_manager.IsInside(airspace) ||
      class_settings.fill_mode ==
      AirspaceClassRendererSettings::FillMode::ALL;

    if (!warning_manager.IsAcked(airspace) &&
        class_settings.fill_mode !=
        AirspaceClassRendererSettings::FillMode::NONE) {
      GLEnable stencil(GL_STENCIL_TEST);

      if (!fill_airspace) {
        // set stencil for filling (bit 0)
        SetFillStencil();
        DrawPrepared();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }

      // fill interior without overpainting any previous outlines
      {
        SetupInterior(airspace, !fill_airspace);
        GLEnable blend(GL_BLEND);
        DrawPrepared();
      }

      if (!fill_airspace) {
        // clear fill stencil (bit 0)
        ClearFillStencil();
        DrawPrepared();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }
    }

    // draw outline
    if (SetupOutline(airspace))
      DrawPrepared();
  }

protected:
  virtual void Visit(const AbstractAirspace &airspace) override {
    switch (airspace.GetShape()) {
    case AbstractAirspace::Shape::CIRCLE:
      VisitCircle((const AirspaceCircle &)airspace);
      break;

    case AbstractAirspace::Shape::POLYGON:
      VisitPolygon((const AirspacePolygon &)airspace);
      break;
    }
  }

private:
  bool SetupOutline(const AbstractAirspace &airspace) {
    AirspaceClass type = airspace.GetType();

    if (settings.black_outline)
      canvas.SelectBlackPen();
    else if (settings.classes[type].border_width == 0)
      // Don't draw outlines if border_width == 0
      return false;
    else
      canvas.Select(look.classes[type].border_pen);

    canvas.SelectHollowBrush();

    // set bit 1 in stencil buffer, where an outline is drawn
    glStencilFunc(GL_ALWAYS, 3, 3);
    glStencilMask(2);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    return true;
  }

  void SetupInterior(const AbstractAirspace &airspace,
                     bool check_fillstencil = false) {
    const AirspaceClassLook &class_look = look.classes[airspace.GetType()];

    // restrict drawing area and don't paint over previously drawn outlines
    if (check_fillstencil)
      glStencilFunc(GL_EQUAL, 1, 3);
    else
      glStencilFunc(GL_EQUAL, 0, 2);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    canvas.Select(Brush(class_look.fill_color.WithAlpha(90)));
    canvas.SelectNullPen();
  }

  void SetFillStencil() {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 3, 3);
    glStencilMask(1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    canvas.SelectHollowBrush();
    canvas.Select(look.thick_pen);
  }

  void ClearFillStencil() {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 3, 3);
    glStencilMask(1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    canvas.SelectHollowBrush();
    canvas.Select(look.thick_pen);
  }
};

class AirspaceFillRenderer final
  : public AirspaceVisitor, protected MapCanvas
{
  const AirspaceLook &look;
  const AirspaceWarningCopy &warning_manager;
  const AirspaceRendererSettings &settings;

public:
  AirspaceFillRenderer(Canvas &_canvas, const WindowProjection &_projection,
                       const AirspaceLook &_look,
                       const AirspaceWarningCopy &_warnings,
                       const AirspaceRendererSettings &_settings)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(fixed(1.1))),
     look(_look), warning_manager(_warnings), settings(_settings)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

private:
  void VisitCircle(const AirspaceCircle &airspace) {
    RasterPoint screen_center = projection.GeoToScreen(airspace.GetCenter());
    unsigned screen_radius = projection.GeoToScreenDistance(airspace.GetRadius());

    if (!warning_manager.IsAcked(airspace) && SetupInterior(airspace)) {
      GLEnable blend(GL_BLEND);
      canvas.DrawCircle(screen_center.x, screen_center.y, screen_radius);
    }

    // draw outline
    if (SetupOutline(airspace))
      canvas.DrawCircle(screen_center.x, screen_center.y, screen_radius);
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
    if (!PreparePolygon(airspace.GetPoints()))
      return;

    if (!warning_manager.IsAcked(airspace) && SetupInterior(airspace)) {
      // fill interior without overpainting any previous outlines
      GLEnable blend(GL_BLEND);
      DrawPrepared();
    }

    // draw outline
    if (SetupOutline(airspace))
      DrawPrepared();
  }

protected:
  virtual void Visit(const AbstractAirspace &airspace) override {
    switch (airspace.GetShape()) {
    case AbstractAirspace::Shape::CIRCLE:
      VisitCircle((const AirspaceCircle &)airspace);
      break;

    case AbstractAirspace::Shape::POLYGON:
      VisitPolygon((const AirspacePolygon &)airspace);
      break;
    }
  }

private:
  bool SetupOutline(const AbstractAirspace &airspace) {
    AirspaceClass type = airspace.GetType();

    if (settings.black_outline)
      canvas.SelectBlackPen();
    else if (settings.classes[type].border_width == 0)
      // Don't draw outlines if border_width == 0
      return false;
    else
      canvas.Select(look.classes[type].border_pen);

    canvas.SelectHollowBrush();

    return true;
  }

  bool SetupInterior(const AbstractAirspace &airspace) {
    if (settings.fill_mode == AirspaceRendererSettings::FillMode::NONE)
      return false;

    const AirspaceClassLook &class_look = look.classes[airspace.GetType()];

    canvas.Select(Brush(class_look.fill_color.WithAlpha(48)));
    canvas.SelectNullPen();

    return true;
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
class AirspaceVisitorMap final
  : public AirspaceVisitor, public StencilMapCanvas
{
  const AirspaceLook &look;
  const AirspaceWarningCopy &warnings;

public:
  AirspaceVisitorMap(StencilMapCanvas &_helper,
                     const AirspaceWarningCopy &_warnings,
                     const AirspaceRendererSettings &_settings,
                     const AirspaceLook &_airspace_look)
    :StencilMapCanvas(_helper),
     look(_airspace_look), warnings(_warnings)
  {
    switch (settings.fill_mode) {
    case AirspaceRendererSettings::FillMode::DEFAULT:
    case AirspaceRendererSettings::FillMode::PADDING:
      use_stencil = !IsAncientHardware();
      break;

    case AirspaceRendererSettings::FillMode::ALL:
    case AirspaceRendererSettings::FillMode::NONE:
      use_stencil = false;
      break;
    }
  }

private:
  void VisitCircle(const AirspaceCircle &airspace) {
    RasterPoint center = proj.GeoToScreen(airspace.GetCenter());
    unsigned radius = proj.GeoToScreenDistance(airspace.GetRadius());
    DrawCircle(center, radius);
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
    DrawSearchPointVector(airspace.GetPoints());
  }

protected:
  virtual void Visit(const AbstractAirspace &airspace) override {
    if (warnings.IsAcked(airspace))
      return;

    AirspaceClass airspace_class = airspace.GetType();
    if (settings.fill_mode == AirspaceRendererSettings::FillMode::NONE ||
        settings.classes[airspace_class].fill_mode ==
        AirspaceClassRendererSettings::FillMode::NONE)
      return;

    Begin();
    SetBufferPens(airspace);

    switch (airspace.GetShape()) {
    case AbstractAirspace::Shape::CIRCLE:
      VisitCircle((const AirspaceCircle &)airspace);
      break;

    case AbstractAirspace::Shape::POLYGON:
      VisitPolygon((const AirspacePolygon &)airspace);
      break;
    }
  }

private:
  void SetBufferPens(const AbstractAirspace &airspace) {
    AirspaceClass airspace_class = airspace.GetType();

#ifndef HAVE_HATCHED_BRUSH
    buffer.Select(look.classes[airspace_class].solid_brush);
#else /* HAVE_HATCHED_BRUSH */

#ifdef HAVE_ALPHA_BLEND
    if (settings.transparency && AlphaBlendAvailable()) {
      buffer.Select(look.classes[airspace_class].solid_brush);
    } else {
#endif
      // this color is used as the black bit
      buffer.SetTextColor(LightColor(look.classes[airspace_class].fill_color));

      // get brush, can be solid or a 1bpp bitmap
      buffer.Select(look.brushes[settings.classes[airspace_class].brush]);

      buffer.SetBackgroundOpaque();
      buffer.SetBackgroundColor(COLOR_WHITE);
#ifdef HAVE_ALPHA_BLEND
    }
#endif
#endif /* HAVE_HATCHED_BRUSH */

    buffer.SelectNullPen();

    if (use_stencil) {
      if (warnings.HasWarning(airspace) || warnings.IsInside(airspace) ||
          settings.classes[airspace_class].fill_mode ==
          AirspaceClassRendererSettings::FillMode::ALL) {
        stencil.SelectBlackBrush();
        stencil.SelectNullPen();
      } else {
        stencil.Select(look.thick_pen);
        stencil.SelectHollowBrush();
      }
    }
  }
};

class AirspaceOutlineRenderer final
  : public AirspaceVisitor, protected MapCanvas
{
  const AirspaceLook &look;
  const AirspaceRendererSettings &settings;

public:
  AirspaceOutlineRenderer(Canvas &_canvas, const WindowProjection &_projection,
                          const AirspaceLook &_look,
                          const AirspaceRendererSettings &_settings)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(fixed(1.1))),
     look(_look), settings(_settings)
  {
    if (settings.black_outline)
      canvas.SelectBlackPen();
    canvas.SelectHollowBrush();
  }

protected:
  bool SetupCanvas(const AbstractAirspace &airspace) {
    if (settings.black_outline)
      return true;

    AirspaceClass type = airspace.GetType();
    if (settings.classes[type].border_width == 0)
      // Don't draw outlines if border_width == 0
      return false;

    canvas.Select(look.classes[type].border_pen);

    return true;
  }

private:
  void VisitCircle(const AirspaceCircle &airspace) {
    DrawCircle(airspace.GetCenter(), airspace.GetRadius());
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
    DrawPolygon(airspace.GetPoints());
  }

public:
  virtual void Visit(const AbstractAirspace &airspace) override {
    if (!SetupCanvas(airspace))
      return;

    switch (airspace.GetShape()) {
    case AbstractAirspace::Shape::CIRCLE:
      VisitCircle((const AirspaceCircle &)airspace);
      break;

    case AbstractAirspace::Shape::POLYGON:
      VisitPolygon((const AirspacePolygon &)airspace);
      break;
    }
  }
};

#endif // !ENABLE_OPENGL

void
AirspaceRenderer::DrawIntersections(Canvas &canvas,
                                    const WindowProjection &projection) const
{
  for (unsigned i = intersections.size(); i--;) {
    RasterPoint sc;
    if (projection.GeoToScreenIfVisible(intersections[i], sc))
      look.intercept_icon.Draw(canvas, sc.x, sc.y);
  }
}

#ifndef ENABLE_OPENGL

inline void
AirspaceRenderer::DrawFill(Canvas &buffer_canvas, Canvas &stencil_canvas,
                           const WindowProjection &projection,
                           const AirspaceRendererSettings &settings,
                           const AirspaceWarningCopy &awc,
                           const AirspacePredicate &visible)
{
  StencilMapCanvas helper(buffer_canvas, stencil_canvas, projection,
                          settings);
  AirspaceVisitorMap v(helper, awc, settings,
                       look);

  // JMW TODO wasteful to draw twice, can't it be drawn once?
  // we are using two draws so borders go on top of everything

  airspaces->VisitWithinRange(projection.GetGeoScreenCenter(),
                                        projection.GetScreenDistanceMeters(),
                                        v, visible);

  v.Commit();
}

inline void
AirspaceRenderer::DrawFillCached(Canvas &canvas, Canvas &stencil_canvas,
                                 const WindowProjection &projection,
                                 const AirspaceRendererSettings &settings,
                                 const AirspaceWarningCopy &awc,
                                 const AirspacePredicate &visible)
{
  if (awc.GetSerial() != last_warning_serial ||
      !fill_cache.Check(projection)) {
    last_warning_serial = awc.GetSerial();

    Canvas &buffer_canvas = fill_cache.Begin(canvas, projection);
    DrawFill(buffer_canvas, stencil_canvas,
             projection, settings, awc, visible);
    fill_cache.Commit(canvas, projection);
  }

#ifdef HAVE_ALPHA_BLEND
#ifdef HAVE_HATCHED_BRUSH
  if (settings.transparency && AlphaBlendAvailable())
#endif
    fill_cache.AlphaBlendTo(canvas, projection, 60);
#ifdef HAVE_HATCHED_BRUSH
  else
#endif
#endif
#ifdef HAVE_HATCHED_BRUSH
    fill_cache.CopyAndTo(canvas, projection);
#endif
}

inline void
AirspaceRenderer::DrawOutline(Canvas &canvas,
                              const WindowProjection &projection,
                              const AirspaceRendererSettings &settings,
                              const AirspacePredicate &visible) const
{
  AirspaceOutlineRenderer outline_renderer(canvas, projection, look, settings);
  airspaces->VisitWithinRange(projection.GetGeoScreenCenter(),
                                        projection.GetScreenDistanceMeters(),
                                        outline_renderer, visible);
}

#endif

void
AirspaceRenderer::Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
                       Canvas &stencil_canvas,
#endif
                       const WindowProjection &projection,
                       const AirspaceRendererSettings &settings,
                       const AirspaceWarningCopy &awc,
                       const AirspacePredicate &visible)
{
  if (airspaces == NULL || airspaces->IsEmpty())
    return;

#ifdef ENABLE_OPENGL
  if (settings.fill_mode == AirspaceRendererSettings::FillMode::ALL ||
      settings.fill_mode == AirspaceRendererSettings::FillMode::NONE) {
    AirspaceFillRenderer renderer(canvas, projection, look, awc,
                                  settings);
    airspaces->VisitWithinRange(projection.GetGeoScreenCenter(),
                                projection.GetScreenDistanceMeters(),
                                renderer, visible);
  } else {
    AirspaceVisitorRenderer renderer(canvas, projection, look, awc,
                                     settings);
    airspaces->VisitWithinRange(projection.GetGeoScreenCenter(),
                                projection.GetScreenDistanceMeters(),
                                renderer, visible);
  }
#else
  if (settings.fill_mode != AirspaceRendererSettings::FillMode::NONE)
    DrawFillCached(canvas, stencil_canvas, projection, settings, awc, visible);

  DrawOutline(canvas, projection, settings, visible);
#endif

  intersections = awc.GetLocations();
}

void
AirspaceRenderer::Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
                       Canvas &stencil_canvas,
#endif
                       const WindowProjection &projection,
                       const AirspaceRendererSettings &settings)
{
  if (airspaces == NULL)
    return;

  AirspaceWarningCopy awc;
  if (warning_manager != NULL)
    awc.Visit(*warning_manager);

  Draw(canvas,
#ifndef ENABLE_OPENGL
       stencil_canvas,
#endif
       projection, settings, awc, AirspacePredicateTrue());
}

void
AirspaceRenderer::Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
                       Canvas &stencil_canvas,
#endif
                       const WindowProjection &projection,
                       const MoreData &basic,
                       const DerivedInfo &calculated,
                       const AirspaceComputerSettings &computer_settings,
                       const AirspaceRendererSettings &settings)
{
  if (airspaces == NULL)
    return;

  AirspaceWarningCopy awc;
  if (warning_manager != NULL)
    awc.Visit(*warning_manager);

  const AircraftState aircraft = ToAircraftState(basic, calculated);
  const AirspaceMapVisible visible(computer_settings, settings,
                                   aircraft, awc);
  Draw(canvas,
#ifndef ENABLE_OPENGL
       stencil_canvas,
#endif
       projection, settings, awc, visible);
}
