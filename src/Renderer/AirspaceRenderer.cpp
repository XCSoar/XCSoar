// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceRenderer.hpp"
#include "AirspaceRendererSettings.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Look/AirspaceLook.hpp"
#include "MapWindow/MapCanvas.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Pen.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceWarningCopy.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "NMEA/Aircraft.hpp"

class AirspaceMapVisible
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

void
AirspaceRenderer::DrawIntersections(Canvas &canvas,
                                    const WindowProjection &projection) const
{
  for (unsigned i = intersections.size(); i--;) {
    if (auto p = projection.GeoToScreenIfVisible(intersections[i]))
      look.intercept_icon.Draw(canvas, *p);
  }
}

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
  if (airspaces == nullptr || airspaces->IsEmpty())
    return;

  DrawInternal(canvas,
#ifndef ENABLE_OPENGL
               stencil_canvas,
#endif
               projection, settings, awc, visible);

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
  if (airspaces == nullptr)
    return;

  AirspaceWarningCopy awc;
  if (warning_manager != nullptr)
    awc.Visit(*warning_manager);

  Draw(canvas,
#ifndef ENABLE_OPENGL
       stencil_canvas,
#endif
       projection, settings, awc, [](const auto &){ return true; });
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
  if (airspaces == nullptr)
    return;

  AirspaceWarningCopy awc;
  if (warning_manager != nullptr)
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

void
AirspaceRenderer::DrawOutlineGeometry(MapCanvas &mc,
                                      const AbstractAirspace &airspace) noexcept
{
  switch (airspace.GetShape()) {
  case AbstractAirspace::Shape::CIRCLE:
    mc.DrawCircle(static_cast<const AirspaceCircle &>(airspace)
                    .GetReferenceLocation(),
                  static_cast<const AirspaceCircle &>(airspace).GetRadius());
    break;

  case AbstractAirspace::Shape::POLYGON:
    mc.DrawPolygon(static_cast<const AirspacePolygon &>(airspace).GetPoints());
    break;
  }
}

void
AirspaceRenderer::DrawOutlineHighlight(Canvas &canvas,
                                       const WindowProjection &proj,
                                       const AbstractAirspace &airspace,
                                       const AirspaceRendererSettings &settings) noexcept
{
  canvas.SelectHollowBrush();
  canvas.Select(Pen(Layout::ScalePenWidth(3),
                    settings.black_outline ? COLOR_BLACK : COLOR_YELLOW));

  MapCanvas mc(canvas, proj, proj.GetScreenBounds().Scale(1.1));
  DrawOutlineGeometry(mc, airspace);
}
