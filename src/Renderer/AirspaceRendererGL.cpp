/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifdef ENABLE_OPENGL

#include "AirspaceLabelList.hpp"
#include "AirspaceRenderer.hpp"
#include "AirspaceRendererSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "MapWindow/MapCanvas.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceVisitor.hpp"
#include "Airspace/AirspaceWarningCopy.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Screen/OpenGL/Scope.hpp"
#include "Sizes.h"

class AirspaceVisitorRenderer final
  : public AirspaceVisitor, protected MapCanvas
{
  const AirspaceLook &look;
  const AirspaceWarningCopy &warning_manager;
  const AirspaceRendererSettings &settings;
  AirspaceLabelList &labels;

public:
  AirspaceVisitorRenderer(Canvas &_canvas, const WindowProjection &_projection,
                          const AirspaceLook &_look,
                          const AirspaceWarningCopy &_warnings,
                          const AirspaceRendererSettings &_settings,
                          AirspaceLabelList &_labels)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(fixed(1.1))),
     look(_look), warning_manager(_warnings), settings(_settings), labels(_labels)
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

    RasterPoint screen_center = projection.GeoToScreen(airspace.GetReferenceLocation());
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

      // add label only to polygons
      labels.Add(airspace.GetCenter(), airspace.GetBase(), airspace.GetTop());
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
  AirspaceLabelList &labels;

public:
  AirspaceFillRenderer(Canvas &_canvas, const WindowProjection &_projection,
                       const AirspaceLook &_look,
                       const AirspaceWarningCopy &_warnings,
                       const AirspaceRendererSettings &_settings,
                       AirspaceLabelList &_labels)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(fixed(1.1))),
     look(_look), warning_manager(_warnings), settings(_settings), labels(_labels)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

private:
  void VisitCircle(const AirspaceCircle &airspace) {
    RasterPoint screen_center = projection.GeoToScreen(airspace.GetReferenceLocation());
    unsigned screen_radius = projection.GeoToScreenDistance(airspace.GetRadius());

    if (!warning_manager.IsAcked(airspace) && SetupInterior(airspace)) {
      const GLEnable<GL_BLEND> blend;
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
      GLEnable<GL_BLEND> blend;
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

      // add label only to polygons
      labels.Add(airspace.GetCenter(), airspace.GetBase(), airspace.GetTop());
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

void
AirspaceRenderer::DrawInternal(Canvas &canvas,
                               const WindowProjection &projection,
                               const AirspaceRendererSettings &settings,
                               const AirspaceWarningCopy &awc,
                               const AirspacePredicate &visible)
{
  AirspaceLabelList labels;

  // airspaces
  if (settings.fill_mode == AirspaceRendererSettings::FillMode::ALL ||
      settings.fill_mode == AirspaceRendererSettings::FillMode::NONE) {
    AirspaceFillRenderer renderer(canvas, projection, look, awc,
                                  settings, labels);
    airspaces->VisitWithinRange(projection.GetGeoScreenCenter(),
                                projection.GetScreenDistanceMeters(),
                                renderer, visible);
  } else {
    AirspaceVisitorRenderer renderer(canvas, projection, look, awc,
                                     settings, labels);
    airspaces->VisitWithinRange(projection.GetGeoScreenCenter(),
                                projection.GetScreenDistanceMeters(),
                                renderer, visible);
  }

  // labels
  if(settings.label_selection == AirspaceRendererSettings::LabelSelection::ALL)
  {
    labels.Sort();

    // default paint settings
    canvas.SetTextColor(look.label_text_color);
    canvas.Select(*look.name_font);
    canvas.Select(look.label_pen);
    canvas.Select(look.label_brush);

    // draw labels
    TCHAR topText[NAME_SIZE + 1];
    TCHAR baseText[NAME_SIZE + 1];

    for (const auto &label : labels) {
      // size of text
      AirspaceFormatter::FormatAltitudeShort(topText, label.top, false);
      PixelSize topSize = canvas.CalcTextSize(topText);
      AirspaceFormatter::FormatAltitudeShort(baseText, label.base, false);
      PixelSize baseSize = canvas.CalcTextSize(baseText);
      int labelWidth = std::max(topSize.cx, baseSize.cx) +
                       2 * Layout::GetTextPadding();
      int labelHeight = topSize.cy + baseSize.cy;

      // box
      RasterPoint pos = projection.GeoToScreen(label.pos);
      PixelRect rect;
      rect.left = pos.x - labelWidth / 2;
      rect.top = pos.y;
      rect.right = rect.left + labelWidth;
      rect.bottom = rect.top + labelHeight;
      canvas.Rectangle(rect.left, rect.top, rect.right, rect.bottom);
      canvas.DrawHLine(rect.left + Layout::GetTextPadding(),
                       rect.right - Layout::GetTextPadding(),
                       rect.top + labelHeight / 2, look.label_pen.GetColor());

      // top text
      int x = rect.right - Layout::GetTextPadding() - topSize.cx;
      int y = rect.top;
      canvas.DrawText(x, y, topText);

      // base text
      x = rect.right - Layout::GetTextPadding() - baseSize.cx;
      y = rect.bottom - baseSize.cy;
      canvas.DrawText(x, y, baseText);
    }
  }
}

#endif /* ENABLE_OPENGL */
