// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OverlayBitmap.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/opengl/Texture.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#include "ui/canvas/opengl/ConstantAlpha.hpp"
#include "MapSettings.hpp"
#include "ui/canvas/opengl/VertexPointer.hpp"
#include "Projection/WindowProjection.hpp"
#include "Math/Point2D.hpp"
#include "Math/Quadrilateral.hpp"
#include "Math/Boost/Point.hpp"
#include "system/Path.hpp"
#include "util/StaticArray.hxx"

#include <algorithm>

#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/multi_polygon.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/strategies/strategies.hpp>

using ArrayQuadrilateral = StaticArray<DoublePoint2D, 5>;
BOOST_GEOMETRY_REGISTER_RING(ArrayQuadrilateral);

using ClippedPolygon = boost::geometry::model::polygon<DoublePoint2D>;

using ClippedMultiPolygon =
  boost::geometry::model::multi_polygon<ClippedPolygon>;

#ifdef USE_GEOTIFF
MapOverlayBitmap::MapOverlayBitmap(Path path)
#else
[[noreturn]] MapOverlayBitmap::MapOverlayBitmap(Path path)
#endif
  :label((path.GetBase() != nullptr ? path.GetBase() : path).c_str())
{
  grid = bitmap.LoadGeoFile(path);
  bounds = grid.GetCornerQuadrilateral();
  simple_bounds = bounds.GetBounds();
}

/**
 * Convert a GeoPoint to a "fake" flat DoublePoint2D.  This conversion
 * is flawed in many ways, but good enough for clipping polygons.
 */
static constexpr DoublePoint2D
GeoTo2D(GeoPoint p) noexcept
{
  return {p.longitude.Native(), p.latitude.Native()};
}

/**
 * Inverse of GeoTo2D().
 */
static constexpr GeoPoint
GeoFrom2D(DoublePoint2D p) noexcept
{
  return {Angle::Native(p.x), Angle::Native(p.y)};
}

/**
 * Convert a #GeoBounds instance to a boost::geometry box.
 */
[[gnu::const]]
static boost::geometry::model::box<DoublePoint2D>
ToBox(const GeoBounds b) noexcept
{
  return {GeoTo2D(b.GetSouthWest()), GeoTo2D(b.GetNorthEast())};
}

/**
 * Convert a #GeoQuadrilateral instance to a boost::geometry ring.
 */
[[gnu::const]]
static ArrayQuadrilateral
ToArrayQuadrilateral(const GeoQuadrilateral q) noexcept
{
  return {GeoTo2D(q.top_left), GeoTo2D(q.top_right),
      GeoTo2D(q.bottom_right), GeoTo2D(q.bottom_left),
      /* close the ring: */
      GeoTo2D(q.top_left) };
}

/**
 * Clip the quadrilateral inside the screen bounds.
 */
[[gnu::pure]]
static ClippedMultiPolygon
Clip(const GeoQuadrilateral &_geo, const GeoBounds &_bounds) noexcept
{
  const auto geo = ToArrayQuadrilateral(_geo);
  const auto bounds = ToBox(_bounds);

  ClippedMultiPolygon clipped;

  try {
    boost::geometry::intersection(geo, bounds, clipped);
  } catch (const boost::geometry::exception &) {
    /* this can (theoretically) occur with self-intersecting
       geometries; in that case, return an empty polygon */
  }

  return clipped;
}

[[gnu::pure]]
static DoublePoint2D
MapInQuadrilateral(const GeoQuadrilateral &q, const GeoPoint p) noexcept
{
  return MapInQuadrilateral(GeoTo2D(q.top_left), GeoTo2D(q.top_right),
                            GeoTo2D(q.bottom_right), GeoTo2D(q.bottom_left),
                            GeoTo2D(p));
}

bool
MapOverlayBitmap::IsInside(GeoPoint p) const noexcept
{
  return simple_bounds.IsInside(p) &&
    boost::geometry::covered_by(GeoTo2D(p), ToArrayQuadrilateral(bounds));
}

void
MapOverlayBitmap::Draw([[maybe_unused]] Canvas &canvas,
                       [[maybe_unused]] const WindowProjection &projection) noexcept
{
  if (!simple_bounds.Overlaps(projection.GetScreenBounds()))
    /* not visible, outside of screen area */
    return;

  auto clipped = Clip(bounds, projection.GetScreenBounds());
  if (clipped.empty())
    return;

  GLTexture &texture = *bitmap.GetNative();
  const PixelSize allocated = texture.GetAllocatedSize();
  const double x_factor = double(texture.GetWidth()) / allocated.width;
  const double y_factor = double(texture.GetHeight()) / allocated.height;

  Point2D<GLfloat> coord[16];
  BulkPixelPoint vertices[16];

  const ScopeVertexPointer vp(vertices);

  texture.Bind();

  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, coord);

  const auto draw_polygons = [&]() {
    for (const auto &polygon : clipped) {
      const auto &ring = polygon.outer();

      size_t n = ring.size();
      if (ring.front() == ring.back())
        --n;

      for (size_t i = 0; i < n; ++i) {
        const auto v = GeoFrom2D(ring[i]);

        auto p = MapInQuadrilateral(bounds, v);

        double tx = p.x * x_factor;
        double ty = p.y * y_factor;

        if (bitmap.IsFlipped())
          /* flip within the image's valid texture region, not the
             whole allocated texture: when the texture is padded to a
             power-of-two (no GL_..._npot), y_factor < 1, and flipping
             around 1.0 would sample the uninitialised padding */
          ty = y_factor - ty;

        /* clamp to the valid texture region: when the texture is
           padded to a power-of-two, the area beyond [x_factor,y_factor]
           is uninitialised. This clamp avoids a sampling outside the
           initialized region, that would cause a {-1,-1} in
           MapInQuadrilateral and also against texel bleed at edge */
        coord[i].x = std::clamp(tx, 0.0, double(x_factor));
        coord[i].y = std::clamp(ty, 0.0, double(y_factor));

        vertices[i] = projection.GeoToScreen(v);
      }

      glDrawArrays(GL_TRIANGLE_FAN, 0, n);
    }
  };

  if (blend_mode == MapOverlayBlendMode::ADD) {
    const ScopeTextureMultiplyAlpha blend(alpha);
    draw_polygons();
  } else {
    const ScopeTextureConstantAlpha blend(use_bitmap_alpha, alpha);
    draw_polygons();
  }

  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
}
