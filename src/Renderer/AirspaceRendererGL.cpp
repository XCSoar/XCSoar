// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef ENABLE_OPENGL

#include "AirspaceRenderer.hpp"
#include "AirspaceRendererSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"
#include "MapWindow/MapCanvas.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceWarningCopy.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"
#include "ui/canvas/opengl/Scope.hpp"

class AirspaceVisitorRenderer final
  : protected MapCanvas
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
               _projection.GetScreenBounds().Scale(1.1)),
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
	AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();
    const AirspaceClassRendererSettings &class_settings =
      settings.classes[as_type_or_class];
    const AirspaceClassLook &class_look = look.classes[as_type_or_class];

    auto screen_center = projection.GeoToScreen(airspace.GetReferenceLocation());
    unsigned screen_radius = projection.GeoToScreenDistance(airspace.GetRadius());

    if (!warning_manager.IsAcked(airspace) &&
        class_settings.fill_mode !=
        AirspaceClassRendererSettings::FillMode::NONE) {
      const GLEnable<GL_STENCIL_TEST> stencil;
      const GLEnable<GL_BLEND> blend;
      SetupInterior(airspace);
      if (warning_manager.HasWarning(airspace) ||
          warning_manager.IsInside(airspace) ||
          look.thick_pen.GetWidth() >= 2 * screen_radius ||
          class_settings.fill_mode ==
          AirspaceClassRendererSettings::FillMode::ALL) {
        // fill whole circle
        canvas.DrawCircle(screen_center, screen_radius);
      } else {
        // draw a ring inside the circle
        Color color = class_look.fill_color;
        Pen pen_donut(look.thick_pen.GetWidth() / 2, color.WithAlpha(90));
        canvas.SelectHollowBrush();
        canvas.Select(pen_donut);
        canvas.DrawCircle(screen_center,
                          screen_radius - look.thick_pen.GetWidth() / 4);

        if (warning_manager.IsWarningCapable(airspace) &&
            !warning_manager.IsClearedAtCurrentAltitude(airspace)) {
          /* Second pass: fill cleared-airspace ring pixels (bit 3)
             inside this circle, completing the padding at the
             cleared/active boundary. Bit 0 must be clear (those
             pixels are already drawn above), bit 2 must be clear (not
             inside the cleared area), and bit 1 must be clear (not on
             an outline). Skipped for purely informative airspaces and
             for the cleared airspace itself. */
          canvas.Select(Brush(color.WithAlpha(90)));
          canvas.SelectNullPen();
          glStencilFunc(GL_EQUAL, 8, 3 | 4 | 8);
          canvas.DrawCircle(screen_center, screen_radius);
        }
      }
    }

    // draw outline
    if (SetupOutline(airspace))
      canvas.DrawCircle(screen_center, screen_radius);
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
	AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();
    if (!PreparePolygon(airspace.GetPoints()))
      return;

    const AirspaceClassRendererSettings &class_settings =
      settings.classes[as_type_or_class];

    bool fill_airspace = warning_manager.HasWarning(airspace) ||
      warning_manager.IsInside(airspace) ||
      class_settings.fill_mode ==
      AirspaceClassRendererSettings::FillMode::ALL;

    if (!warning_manager.IsAcked(airspace) &&
        class_settings.fill_mode !=
        AirspaceClassRendererSettings::FillMode::NONE) {
      const GLEnable<GL_STENCIL_TEST> stencil;

      if (!fill_airspace) {
        // set stencil for filling (bit 0)
        SetFillStencil();
        DrawPrepared();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }

      // fill interior without overpainting any previous outlines
      {
        SetupInterior(airspace, !fill_airspace);
        const GLEnable<GL_BLEND> blend;
        DrawPrepared();

        if (!fill_airspace && warning_manager.IsWarningCapable(airspace) &&
            !warning_manager.IsClearedAtCurrentAltitude(airspace)) {
          /* Second pass: also fill where the cleared-airspace ring
             (bit 3) overlaps this airspace's geometry, but only where
             the pixel isn't already covered by bit 0 (to avoid double
             blending), not inside a cleared area (bit 2), and not on
             an outline (bit 1). Skipped for purely informative
             airspaces and for the cleared airspace itself. */
          glStencilFunc(GL_EQUAL, 8, 3 | 4 | 8);
          DrawPrepared();
        }
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

public:
  void Visit(const AbstractAirspace &airspace) {
    switch (airspace.GetShape()) {
    case AbstractAirspace::Shape::CIRCLE:
      VisitCircle((const AirspaceCircle &)airspace);
      break;

    case AbstractAirspace::Shape::POLYGON:
      VisitPolygon((const AirspacePolygon &)airspace);
      break;
    }
  }

  /**
   * Pre-pass: For every cleared airspace, stamp its full area into
   * stencil bit 2 (mask=4) and its thick-pen ring area into stencil
   * bit 3 (mask=8). Bit 2 suppresses interior fill for warning 
   * airspaces; bit 3 is used to extend the padding ring of overlapping 
   * active airspaces across the cleared/active boundary.
   */
  void VisitClearance(const AbstractAirspace &airspace) {
    if (!warning_manager.IsClearedAtCurrentAltitude(airspace))
      return;

    const GLEnable<GL_STENCIL_TEST> stencil;

    switch (airspace.GetShape()) {
    case AbstractAirspace::Shape::CIRCLE: {
      const auto &circle = (const AirspaceCircle &)airspace;
      auto screen_center =
        projection.GeoToScreen(circle.GetReferenceLocation());
      unsigned screen_radius =
        projection.GeoToScreenDistance(circle.GetRadius());

      // bit 2: full circle area
      SetClearanceStencil();
      canvas.DrawCircle(screen_center, screen_radius);

      // bit 3: thick-pen ring along the circle border
      SetClearedRingStencil();
      canvas.DrawCircle(screen_center, screen_radius);
      break;
    }

    case AbstractAirspace::Shape::POLYGON: {
      const auto &polygon = (const AirspacePolygon &)airspace;
      if (!PreparePolygon(polygon.GetPoints()))
        break;

      // bit 2: full polygon area
      SetClearanceStencil();
      DrawPrepared();

      // bit 3: thick-pen ring along the polygon border
      SetClearedRingStencil();
      DrawPrepared();
      break;
    }
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  }

private:
  bool SetupOutline(const AbstractAirspace &airspace) {
    AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();

    if (settings.black_outline)
      canvas.SelectBlackPen();
    else if (settings.classes[as_type_or_class].border_width == 0)
      // Don't draw outlines if border_width == 0
      return false;
    else
      canvas.Select(look.classes[as_type_or_class].border_pen);

    canvas.SelectHollowBrush();

    // set bit 1 in stencil buffer, where an outline is drawn
    glStencilFunc(GL_ALWAYS, 3, 3);
    glStencilMask(2);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    return true;
  }

  void SetupInterior(const AbstractAirspace &airspace,
                     bool check_fillstencil = false) {
	AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();
    const AirspaceClassLook &class_look = look.classes[as_type_or_class];

    /* Restrict drawing area: don't paint over previously drawn
       outlines (bit 1) and don't paint inside cleared airspaces
       (bit 2, set by DrawClearancePrePass). */
    if (check_fillstencil)
      glStencilFunc(GL_EQUAL, 1, 3 | 4);
    else
      glStencilFunc(GL_EQUAL, 0, 2 | 4);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    canvas.Select(Brush(class_look.fill_color.WithAlpha(90)));
    canvas.SelectNullPen();
  }

  void SetClearanceStencil() {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 4, 4);
    glStencilMask(4);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    canvas.SelectNullPen();
    canvas.SelectBlackBrush();
  }

  /**
   * Set up stencil writes for bit 3 (mask=8): the thick-pen ring
   * area along the border of a cleared airspace.
   */
  void SetClearedRingStencil() {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 8, 8);
    glStencilMask(8);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    canvas.SelectHollowBrush();
    canvas.Select(look.thick_pen);
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
  : protected MapCanvas
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
               _projection.GetScreenBounds().Scale(1.1)),
     look(_look), warning_manager(_warnings), settings(_settings)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

private:
  void VisitCircle(const AirspaceCircle &airspace) {
    auto screen_center = projection.GeoToScreen(airspace.GetReferenceLocation());
    unsigned screen_radius = projection.GeoToScreenDistance(airspace.GetRadius());

    if (!warning_manager.IsAcked(airspace) && SetupInterior(airspace)) {
      const GLEnable<GL_BLEND> blend;
      canvas.DrawCircle(screen_center, screen_radius);
    }

    // draw outline
    if (SetupOutline(airspace))
      canvas.DrawCircle(screen_center, screen_radius);
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
    if (!PreparePolygon(airspace.GetPoints()))
      return;

    if (!warning_manager.IsAcked(airspace) && SetupInterior(airspace)) {
      // fill interior without overpainting any previous outlines
      GLEnable<GL_BLEND> blend;
      DrawPrepared();
    }

    // draw outline
    if (SetupOutline(airspace))
      DrawPrepared();
  }

public:
  void Visit(const AbstractAirspace &airspace) {
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
    AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();

    if (settings.black_outline)
      canvas.SelectBlackPen();
    else if (settings.classes[as_type_or_class].border_width == 0)
      // Don't draw outlines if border_width == 0
      return false;
    else
      canvas.Select(look.classes[as_type_or_class].border_pen);

    canvas.SelectHollowBrush();

    return true;
  }

  bool SetupInterior(const AbstractAirspace &airspace) {
	AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();
    if (settings.fill_mode == AirspaceRendererSettings::FillMode::NONE)
      return false;

    const AirspaceClassLook &class_look = look.classes[as_type_or_class];

    canvas.Select(Brush(class_look.fill_color.WithAlpha(48)));
    canvas.SelectNullPen();

    return true;
  }
};

void
AirspaceRenderer::DrawInternal(Canvas &canvas,
                               const WindowProjection &projection,
                               const AirspaceRendererSettings &settings,
                               const AirspaceWarningCopy &awc,
                               const AirspacePredicate &visible)
{
  const auto range =
    airspaces->QueryWithinRange(projection.GetGeoScreenCenter(),
                                projection.GetScreenDistanceMeters());

  if (settings.fill_mode == AirspaceRendererSettings::FillMode::ALL ||
      settings.fill_mode == AirspaceRendererSettings::FillMode::NONE) {
    AirspaceFillRenderer renderer(canvas, projection, look, awc, settings);
    for (const auto &i : range) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (visible(airspace))
        renderer.Visit(airspace);
    }
  } else {
    AirspaceVisitorRenderer renderer(canvas, projection, look, awc, settings);

    // Init pass: stamp cleared airspaces into stencil bit 2
    for (const auto &i : range) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (visible(airspace))
        renderer.VisitClearance(airspace);
    }

    // Main pass: draw warnings and outlines
    for (const auto &i : range) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (visible(airspace))
        renderer.Visit(airspace);
    }
  }
}

#endif /* ENABLE_OPENGL */
