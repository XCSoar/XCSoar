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

#include "OZPreviewRenderer.hpp"
#include "OZRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Features.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Engine/Task/ObservationZones/Boundary.hpp"
#include "Projection/WindowProjection.hpp"
#include "Asset.hpp"

void
OZPreviewRenderer::Draw(Canvas &canvas, const ObservationZonePoint &oz,
                        const PixelPoint pt, unsigned radius,
                        const TaskLook &look,
                        const AirspaceRendererSettings &airspace_settings,
                        const AirspaceLook &airspace_look)
{
  double scale;
  GeoPoint center;

  if (IsAncientHardware()) {
    scale = double(radius) / ((const CylinderZone &)oz).GetRadius();
    center = oz.GetReference();
  } else {
    OZBoundary boundary = oz.GetBoundary();

    GeoBounds bounds = GeoBounds::Invalid();
    for (auto i = boundary.begin(), end = boundary.end(); i != end; ++i)
      bounds.Extend(*i);

    center = bounds.GetCenter();

    auto geo_width = bounds.GetGeoWidth();
    auto geo_heigth = bounds.GetGeoHeight();

    scale = double(radius * 2) / std::max(geo_heigth, geo_width);
  }

  WindowProjection projection;
  projection.SetScreenSize({radius * 2, radius * 2});
  projection.SetScreenOrigin(pt.x, pt.y);
  projection.SetGeoLocation(center);
  projection.SetScale(scale);
  projection.SetScreenAngle(Angle::Zero());
  projection.UpdateScreenBounds();

  OZRenderer ozv(look, airspace_look, airspace_settings);
  ozv.Draw(canvas, OZRenderer::LAYER_SHADE, projection, oz, 1);
  ozv.Draw(canvas, OZRenderer::LAYER_INACTIVE, projection, oz, 1);
  ozv.Draw(canvas, OZRenderer::LAYER_ACTIVE, projection, oz, 1);
}
