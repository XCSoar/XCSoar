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

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

static void
DrawPolygon(Canvas &canvas, const AirspacePolygon &airspace,
            const RasterPoint pt, unsigned radius)
{
  if (IsAncientHardware()) {
    canvas.Rectangle(pt.x - radius, pt.y - radius,
                     pt.x + radius, pt.y + radius);
    return;
  }

  GeoBounds bounds = airspace.GetGeoBounds();
  GeoPoint center = bounds.GetCenter();

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
  projection.SetScreenAngle(Angle::Zero());
  projection.UpdateScreenBounds();

  const SearchPointVector &border = airspace.GetPoints();

  std::vector<RasterPoint> pts;
  pts.reserve(border.size());
  for (auto it = border.begin(), it_end = border.end(); it != it_end; ++it)
    pts.push_back(projection.GeoToScreen(it->get_location()));

  canvas.DrawPolygon(&pts[0], (unsigned)pts.size());
}

void
AirspacePreviewRenderer::Draw(Canvas &canvas, const AbstractAirspace &airspace,
                              const RasterPoint pt, unsigned radius,
                              const AirspaceRendererSettings &settings,
                              const AirspaceLook &look)
{
  AirspaceClass type = airspace.GetType();
  const AirspaceClassRendererSettings &class_settings = settings.classes[type];

  if (class_settings.fill_mode !=
      AirspaceClassRendererSettings::FillMode::NONE) {
#ifdef ENABLE_OPENGL
    GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Color color = class_settings.fill_color;
    canvas.Select(Brush(color.WithAlpha(48)));
#elif defined(ENABLE_SDL)
    Color color = class_settings.fill_color;
    canvas.Select(Brush(LightColor(color)));
#else
    canvas.Select(look.brushes[settings.transparency ?
                               3 : class_settings.brush]);
    canvas.SetTextColor(LightColor(class_settings.fill_color));
    canvas.SetMixMask();
#endif

    canvas.SelectNullPen();
    if (airspace.GetShape() == AbstractAirspace::Shape::CIRCLE)
      canvas.DrawCircle(pt.x, pt.y, radius);
    else
      DrawPolygon(canvas, (const AirspacePolygon &)airspace, pt, radius);

#ifdef ENABLE_OPENGL
    glDisable(GL_BLEND);
#elif !defined(ENABLE_SDL)
    canvas.SetMixCopy();
#endif
  }

  if (settings.black_outline)
    canvas.SelectBlackPen();
  else if (class_settings.border_width == 0)
    // Don't draw outlines if border_width == 0
    return;
  else
    canvas.Select(look.pens[type]);

  canvas.SelectHollowBrush();

  if (airspace.GetShape() == AbstractAirspace::Shape::CIRCLE)
    canvas.DrawCircle(pt.x, pt.y, radius);
  else
    DrawPolygon(canvas, (const AirspacePolygon &)airspace, pt, radius);
}
