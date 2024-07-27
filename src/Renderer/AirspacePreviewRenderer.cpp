// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspacePreviewRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Features.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Look/AirspaceLook.hpp"
#include "Geo/GeoBounds.hpp"
#include "Projection/WindowProjection.hpp"

#include <vector>

static void
GetPolygonPoints(std::vector<BulkPixelPoint> &pts,
                 const AirspacePolygon &airspace,
                 const PixelPoint pt, unsigned radius)
{
  GeoBounds bounds = airspace.GetGeoBounds();
  GeoPoint center = bounds.GetCenter();

  auto geo_heigth = bounds.GetGeoHeight();
  auto geo_width = bounds.GetGeoWidth();

  auto geo_size = std::max(geo_heigth, geo_width);

  WindowProjection projection;
  projection.SetScreenSize({radius * 2, radius * 2});
  projection.SetScreenOrigin(pt);
  projection.SetGeoLocation(center);
  projection.SetScale(radius * 2 / geo_size);
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
AirspacePreviewRenderer::UnprepareFill([[maybe_unused]] Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  ::glDisable(GL_BLEND);
#elif defined(USE_GDI)
  canvas.SetTextColor(COLOR_BLACK);
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
DrawShape(Canvas &canvas, AbstractAirspace::Shape shape, const PixelPoint pt,
          unsigned radius, const std::vector<BulkPixelPoint> &pts)
{
  if (shape == AbstractAirspace::Shape::CIRCLE)
    canvas.DrawCircle(pt, radius);
  else
    canvas.DrawPolygon(&pts[0], (unsigned)pts.size());
}

void
AirspacePreviewRenderer::Draw(Canvas &canvas, const AbstractAirspace &airspace,
                              const PixelPoint pt, unsigned radius,
                              const AirspaceRendererSettings &settings,
                              const AirspaceLook &look)
{
  AbstractAirspace::Shape shape = airspace.GetShape();
  AirspaceClass asclass = airspace.GetType() == AirspaceClass::OTHER ? airspace.GetClass() : airspace.GetType();

  // Container for storing the points of a polygon airspace
  std::vector<BulkPixelPoint> pts;
  if (shape == AbstractAirspace::Shape::POLYGON)
    GetPolygonPoints(pts, (const AirspacePolygon &)airspace, pt, radius);

  if (PrepareFill(canvas, asclass, look, settings)) {
    DrawShape(canvas, shape, pt, radius, pts);
    UnprepareFill(canvas);
  }

  if (PrepareOutline(canvas, asclass, look, settings))
    DrawShape(canvas, shape, pt, radius, pts);
}
