// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OZPreviewRenderer.hpp"
#include "OZRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Projection/WindowProjection.hpp"
#include "Asset.hpp"

void
OZPreviewRenderer::Draw(Canvas &canvas, const ObservationZonePoint &oz,
                        const PixelPoint pt, unsigned radius,
                        const TaskLook &look,
                        const AirspaceRendererSettings &airspace_settings,
                        const AirspaceLook &airspace_look)
{
  const auto bounds = OZRenderer::GetGeoBounds(oz);

  const GeoPoint center = bounds.GetCenter();

  auto geo_width = bounds.GetGeoWidth();
  auto geo_heigth = bounds.GetGeoHeight();

  const double scale = double(radius * 2) / std::max(geo_heigth, geo_width);

  WindowProjection projection;
  projection.SetScreenSize({radius * 2, radius * 2});
  projection.SetScreenOrigin(pt);
  projection.SetGeoLocation(center);
  projection.SetScale(scale);
  projection.SetScreenAngle(Angle::Zero());
  projection.UpdateScreenBounds();

  OZRenderer ozv(look, airspace_look, airspace_settings);
  ozv.Draw(canvas, OZRenderer::LAYER_SHADE, projection, oz, 1);
  ozv.Draw(canvas, OZRenderer::LAYER_INACTIVE, projection, oz, 1);
  ozv.Draw(canvas, OZRenderer::LAYER_ACTIVE, projection, oz, 1);
}
