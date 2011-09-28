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

#include "OZPreviewRenderer.hpp"
#include "OZRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Features.hpp"
#include "Engine/Task/Tasks/BaseTask/ObservationZonePoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Look/AirspaceLook.hpp"
#include "Look/TaskLook.hpp"
#include "Projection/WindowProjection.hpp"
#include "Asset.hpp"

void
OZPreviewRenderer::Draw(Canvas &canvas, const ObservationZonePoint &oz,
                        const RasterPoint pt, unsigned radius,
                        const TaskLook &look,
                        const AirspaceRendererSettings &airspace_settings,
                        const AirspaceLook &airspace_look)
{
  fixed scale;
  GeoPoint center;

  if (is_ancient_hardware()) {
    scale = fixed(radius) / ((const CylinderZone &)oz).getRadius();
    center = oz.get_location();
  } else {
    GeoBounds bounds(oz.GetBoundaryParametric(fixed_one));
    for (fixed i = fixed_zero; i < fixed_one; i += fixed_one / 10)
      bounds.extend(oz.GetBoundaryParametric(i));

    center = bounds.center();

    fixed geo_heigth = GeoPoint(center.longitude, bounds.north).Distance(
                       GeoPoint(center.longitude, bounds.south));
    fixed geo_width = GeoPoint(bounds.west, center.latitude).Distance(
                      GeoPoint(bounds.east, center.latitude));

    scale = fixed(radius * 2) / std::max(geo_heigth, geo_width);
  }

  WindowProjection projection;
  projection.SetScreenSize(radius * 2, radius * 2);
  projection.SetScreenOrigin(pt.x, pt.y);
  projection.SetGeoLocation(center);
  projection.SetScale(scale);
  projection.SetScreenAngle(Angle::zero());
  projection.UpdateScreenBounds();

  OZRenderer ozv(look, airspace_look, airspace_settings);
  ozv.Draw(canvas, OZRenderer::LAYER_SHADE, projection, oz, 1);
  ozv.Draw(canvas, OZRenderer::LAYER_INACTIVE, projection, oz, 1);
  ozv.Draw(canvas, OZRenderer::LAYER_ACTIVE, projection, oz, 1);
}
