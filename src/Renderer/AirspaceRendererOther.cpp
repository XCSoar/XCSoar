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

#ifndef ENABLE_OPENGL

#include "AirspaceRenderer.hpp"
#include "AirspaceRendererSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Features.hpp"
#include "MapWindow/MapCanvas.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceWarningCopy.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"
#include "MapWindow/StencilMapCanvas.hpp"
#include "Asset.hpp"

/**
 * Class to render airspaces onto map in two passes,
 * one for border, one for area.
 * This is a bit slow because projections are performed twice.
 * The old way of doing it was possibly faster but required a lot
 * of code overhead.
 */
class AirspaceVisitorMap final
  : public StencilMapCanvas
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
    auto center = proj.GeoToScreen(airspace.GetReferenceLocation());
    unsigned radius = proj.GeoToScreenDistance(airspace.GetRadius());
    DrawCircle(center, radius);
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
    DrawSearchPointVector(airspace.GetPoints());
  }

public:
  void Visit(const AbstractAirspace &airspace) {
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
    if (settings.transparency) {
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
  : protected MapCanvas
{
  const AirspaceLook &look;
  const AirspaceRendererSettings &settings;

public:
  AirspaceOutlineRenderer(Canvas &_canvas, const WindowProjection &_projection,
                          const AirspaceLook &_look,
                          const AirspaceRendererSettings &_settings)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(1.1)),
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
    DrawCircle(airspace.GetReferenceLocation(), airspace.GetRadius());
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
    DrawPolygon(airspace.GetPoints());
  }

public:
  void Visit(const AbstractAirspace &airspace) {
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

inline bool
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

  const auto range =
    airspaces->QueryWithinRange(projection.GetGeoScreenCenter(),
                                projection.GetScreenDistanceMeters());
  for (const auto &i : range) {
    const AbstractAirspace &airspace = i.GetAirspace();
    if (visible(airspace))
      v.Visit(airspace);
  }

  return v.Commit();
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
    if (DrawFill(buffer_canvas, stencil_canvas,
                                projection, settings, awc, visible))
      fill_cache.Commit(canvas, projection);
    else
      fill_cache.CommitEmpty();
  }

#ifdef HAVE_ALPHA_BLEND
#ifdef HAVE_HATCHED_BRUSH
  if (settings.transparency)
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
  const auto range =
    airspaces->QueryWithinRange(projection.GetGeoScreenCenter(),
                                projection.GetScreenDistanceMeters());

  AirspaceOutlineRenderer outline_renderer(canvas, projection, look, settings);
  for (const auto &i : range) {
    const AbstractAirspace &airspace = i.GetAirspace();
    if (visible(airspace))
      outline_renderer.Visit(airspace);
  }
}

void
AirspaceRenderer::DrawInternal(Canvas &canvas, Canvas &stencil_canvas,
                               const WindowProjection &projection,
                               const AirspaceRendererSettings &settings,
                               const AirspaceWarningCopy &awc,
                               const AirspacePredicate &visible)
{
  if (settings.fill_mode != AirspaceRendererSettings::FillMode::NONE)
    DrawFillCached(canvas, stencil_canvas, projection, settings, awc, visible);

  DrawOutline(canvas, projection, settings, visible);
}

#endif /* ENABLE_OPENGL */
