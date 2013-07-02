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
GetPolygonPoints(std::vector<RasterPoint> &pts,
                 const AirspacePolygon &airspace,
                 const RasterPoint pt, unsigned radius)
{
  GeoBounds bounds = airspace.GetGeoBounds();
  GeoPoint center = bounds.GetCenter();

  fixed geo_heigth = bounds.GetGeoHeight();
  fixed geo_width = bounds.GetGeoWidth();

  fixed geo_size = std::max(geo_heigth, geo_width);

  WindowProjection projection;
  projection.SetScreenSize({radius * 2, radius * 2});
  projection.SetScreenOrigin(pt.x, pt.y);
  projection.SetGeoLocation(center);
  projection.SetScale(fixed(radius * 2) / geo_size);
  projection.SetScreenAngle(Angle::Zero());
  projection.UpdateScreenBounds();

  const SearchPointVector &border = airspace.GetPoints();

  pts.reserve(border.size());
  for (auto it = border.begin(), it_end = border.end(); it != it_end; ++it)
    pts.push_back(projection.GeoToScreen(it->GetLocation()));
}

bool
AirspacePreviewRenderer::PrepareFill(
    Canvas &canvas, AirspaceClass type, const AirspaceLook &look,
    const AirspaceRendererSettings &settings)
{
  const AirspaceClassRendererSettings &class_settings = settings.classes[type];
  const AirspaceClassLook &class_look = look.classes[type];

  if (class_settings.fill_mode ==
      AirspaceClassRendererSettings::FillMode::NONE)
    return false;

#ifdef ENABLE_OPENGL
  ::glEnable(GL_BLEND);
  ::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  Color color = class_look.fill_color;
  canvas.Select(Brush(color.WithAlpha(48)));
#elif defined(USE_MEMORY_CANVAS)
  Color color = class_look.fill_color;
  canvas.Select(Brush(LightColor(color)));
#else
  unsigned brush = class_settings.brush;
#ifdef HAVE_ALPHA_BLEND
  if (settings.transparency)
    brush = 3;
#endif

  canvas.Select(look.brushes[brush]);
  canvas.SetTextColor(LightColor(class_look.fill_color));
  canvas.SetMixMask();
#endif

  canvas.SelectNullPen();

  return true;
}

void
AirspacePreviewRenderer::UnprepareFill(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  ::glDisable(GL_BLEND);
#elif defined(USE_GDI)
  canvas.SetMixCopy();
#endif
}

bool
AirspacePreviewRenderer::PrepareOutline(
    Canvas &canvas, AirspaceClass type, const AirspaceLook &look,
    const AirspaceRendererSettings &settings)
{
  const AirspaceClassRendererSettings &class_settings = settings.classes[type];

  if (settings.black_outline)
    canvas.SelectBlackPen();
  else if (class_settings.border_width == 0)
    // Don't draw outlines if border_width == 0
    return false;
  else
    canvas.Select(look.classes[type].border_pen);

  canvas.SelectHollowBrush();

  return true;
}

static void
DrawShape(Canvas &canvas, AbstractAirspace::Shape shape, const RasterPoint pt,
          unsigned radius, const std::vector<RasterPoint> &pts)
{
  if (shape == AbstractAirspace::Shape::CIRCLE)
    canvas.DrawCircle(pt.x, pt.y, radius);
  else if (IsAncientHardware())
    canvas.Rectangle(pt.x - radius, pt.y - radius,
                     pt.x + radius, pt.y + radius);
  else
    canvas.DrawPolygon(&pts[0], (unsigned)pts.size());
}

void
AirspacePreviewRenderer::Draw(Canvas &canvas, const AbstractAirspace &airspace,
                              const RasterPoint pt, unsigned radius,
                              const AirspaceRendererSettings &settings,
                              const AirspaceLook &look)
{
  AbstractAirspace::Shape shape = airspace.GetShape();
  AirspaceClass type = airspace.GetType();

  // Container for storing the points of a polygon airspace
  std::vector<RasterPoint> pts;
  if (shape == AbstractAirspace::Shape::POLYGON && !IsAncientHardware())
    GetPolygonPoints(pts, (const AirspacePolygon &)airspace, pt, radius);

  if (PrepareFill(canvas, type, look, settings)) {
    DrawShape(canvas, shape, pt, radius, pts);
    UnprepareFill(canvas);
  }

  if (PrepareOutline(canvas, type, look, settings))
    DrawShape(canvas, shape, pt, radius, pts);
}
