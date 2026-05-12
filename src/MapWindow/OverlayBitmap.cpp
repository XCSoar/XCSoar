// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OverlayBitmap.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/opengl/Texture.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#include "ui/canvas/opengl/ConstantAlpha.hpp"
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

MapOverlayBitmap::MapOverlayBitmap(Path path)
  :label((path.GetBase() != nullptr ? path.GetBase() : path).c_str())
{
  bounds = bitmap.LoadGeoFile(path);
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

[[gnu::pure]]
static GeoPoint
InterpolateQuadrilateral(const GeoQuadrilateral &q,
                         double u, double v) noexcept
{
  const auto top = q.top_left.Interpolate(q.top_right, u);
  const auto bottom = q.bottom_left.Interpolate(q.bottom_right, u);
  return top.Interpolate(bottom, v);
}

[[gnu::pure]]
static GeoQuadrilateral
SliceQuadrilateral(const GeoQuadrilateral &q,
                   double u0, double v0,
                   double u1, double v1) noexcept
{
  return {
    InterpolateQuadrilateral(q, u0, v0),
    InterpolateQuadrilateral(q, u1, v0),
    InterpolateQuadrilateral(q, u0, v1),
    InterpolateQuadrilateral(q, u1, v1),
  };
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
  const auto screen_bounds = projection.GetScreenBounds();
  if (!simple_bounds.Overlaps(screen_bounds))
    /* not visible, outside of screen area */
    return;

  GLTexture &texture = *bitmap.GetNative();
  const PixelSize allocated = texture.GetAllocatedSize();
  const double x_factor = double(texture.GetWidth()) / allocated.width;
  const double y_factor = double(texture.GetHeight()) / allocated.height;

  Point2D<GLfloat> coord[16];
  BulkPixelPoint vertices[16];

  const ScopeVertexPointer vp(vertices);

  texture.Bind();

  const ScopeTextureConstantAlpha blend(use_bitmap_alpha, alpha);

  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, coord);

  if (texture.GetWidth() > 512 || texture.GetHeight() > 512) {
    const unsigned x_steps = std::clamp((texture.GetWidth() + 127u) / 128u,
                                        1u, 32u);
    const unsigned y_steps = std::clamp((texture.GetHeight() + 127u) / 128u,
                                        1u, 32u);

    for (unsigned y = 0; y < y_steps; ++y) {
      const double v0 = double(y) / y_steps;
      const double v1 = double(y + 1) / y_steps;

      for (unsigned x = 0; x < x_steps; ++x) {
        const double u0 = double(x) / x_steps;
        const double u1 = double(x + 1) / x_steps;

        const auto cell = SliceQuadrilateral(bounds, u0, v0, u1, v1);
        if (!cell.GetBounds().Overlaps(screen_bounds))
          continue;

        const GeoPoint geo[4] = {
          cell.top_left,
          cell.top_right,
          cell.bottom_right,
          cell.bottom_left,
        };
        const double uv[4][2] = {
          {u0, v0},
          {u1, v0},
          {u1, v1},
          {u0, v1},
        };

        for (unsigned i = 0; i < 4; ++i) {
          coord[i].x = uv[i][0] * x_factor;
          coord[i].y = (bitmap.IsFlipped() ? 1 - uv[i][1] : uv[i][1]) * y_factor;

          vertices[i] = projection.GeoToScreen(geo[i]);
        }

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      }
    }

    glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
    return;
  }

  auto clipped = Clip(bounds, screen_bounds);
  if (clipped.empty()) {
    glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
    return;
  }

  for (const auto &polygon : clipped) {
    const auto &ring = polygon.outer();

    size_t n = ring.size();
    if (ring.front() == ring.back())
      --n;

    for (size_t i = 0; i < n; ++i) {
      const auto v = GeoFrom2D(ring[i]);

      auto p = MapInQuadrilateral(bounds, v);
      coord[i].x = p.x * x_factor;
      coord[i].y = (bitmap.IsFlipped() ? 1 - p.y : p.y) * y_factor;

      vertices[i] = projection.GeoToScreen(v);
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, n);
  }

  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
}
