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

#include "AirspacePreviewRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Features.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Look/AirspaceLook.hpp"
#include "Geo/GeoBounds.hpp"
#include "Projection/WindowProjection.hpp"
#include "Asset.hpp"

#include <vector>

static void
DrawPolygon(Canvas &canvas, const AirspacePolygon &airspace,
            const RasterPoint pt, unsigned radius)
{
  if (is_ancient_hardware()) {
    canvas.rectangle(pt.x - radius, pt.y - radius,
                     pt.x + radius, pt.y + radius);
    return;
  }

  GeoBounds bounds = airspace.GetGeoBounds();
  GeoPoint center = bounds.center();

  fixed geo_heigth = GeoPoint(center.longitude, bounds.north).Distance(
                     GeoPoint(center.longitude, bounds.south));
  fixed geo_width = GeoPoint(bounds.west, center.latitude).Distance(
                    GeoPoint(bounds.east, center.latitude));

  fixed geo_size = std::max(geo_heigth, geo_width);

  WindowProjection projection;
  projection.SetScreenSize(radius * 2, radius * 2);
  projection.SetScreenOrigin(pt.x, pt.y);
  projection.SetGeoLocation(center);
  projection.SetScale(fixed(radius * 2) / geo_size);
  projection.SetScreenAngle(Angle::zero());
  projection.UpdateScreenBounds();

  const SearchPointVector &border = airspace.GetPoints();

  std::vector<RasterPoint> pts;
  pts.reserve(border.size());
  for (SearchPointVector::const_iterator it = border.begin(),
       it_end = border.end(); it != it_end; ++it)
    pts.push_back(projection.GeoToScreen(it->get_location()));

  canvas.polygon(&pts[0], (unsigned)pts.size());
}

void
AirspacePreviewRenderer::Draw(Canvas &canvas, const AbstractAirspace &airspace,
                              const RasterPoint pt, unsigned radius,
                              const AirspaceRendererSettings &settings,
                              const AirspaceLook &look)
{
  canvas.hollow_brush();

  if (settings.black_outline)
    canvas.black_pen();
  else
    canvas.select(look.pens[airspace.GetType()]);

  if (airspace.shape == AbstractAirspace::CIRCLE)
    canvas.circle(pt.x, pt.y, radius);
  else
    DrawPolygon(canvas, (const AirspacePolygon &)airspace, pt, radius);
}
