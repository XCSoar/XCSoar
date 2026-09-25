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
#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/strategies/strategies.hpp>

using ArrayQuadrilateral = StaticArray<DoublePoint2D, 5>;
BOOST_GEOMETRY_REGISTER_RING(ArrayQuadrilateral);

using ClippedPolygon = boost::geometry::model::polygon<DoublePoint2D>;

using ClippedMultiPolygon =
  boost::geometry::model::multi_polygon<ClippedPolygon>;

MapOverlayBitmap::MapOverlayBitmap(Path path)
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
 *
 * Longitude must not wrap (west native <= east native).  Callers that
 * may pass antimeridian-wrapping bounds must split first.
 */
[[gnu::const]]
static boost::geometry::model::box<DoublePoint2D>
ToBox(const GeoBounds b) noexcept
{
  return {GeoTo2D(b.GetSouthWest()), GeoTo2D(b.GetNorthEast())};
}

/**
 * True when #GeoBounds longitude crosses ±180° in native coordinates
 * (west > east), so a single cartesian box would be inverted/empty.
 */
[[gnu::const]]
static bool
LongitudeWraps(const GeoBounds &b) noexcept
{
  return b.GetWest().Native() > b.GetEast().Native();
}

/**
 * Convert a #GeoQuadrilateral instance to a boost::geometry ring.
 */
[[gnu::const]]
static ArrayQuadrilateral
ToArrayQuadrilateral(const GeoQuadrilateral q) noexcept
{
  ArrayQuadrilateral ring{GeoTo2D(q.top_left), GeoTo2D(q.top_right),
      GeoTo2D(q.bottom_right), GeoTo2D(q.bottom_left),
      /* close the ring: */
      GeoTo2D(q.top_left) };

  /* Normalise the winding order: boost::geometry expects clockwise outer rings;
     a counter-clockwise one makes intersection() return an empty result,
     so normalize the ring just in case.
     This can happen e.g. if a a GeoTIFF is stored "south up" (negative
     ModelPixelScale Y), or if some other unusual geotransform is used.
   */
  boost::geometry::correct(ring);
  return ring;
}

/**
 * Intersect @p geo with one non-wrapping screen box; append into @p out.
 */
static void
ClipAgainstBox(const ArrayQuadrilateral &geo,
               const GeoBounds &box,
               ClippedMultiPolygon &out) noexcept
{
  ClippedMultiPolygon piece;
  try {
    boost::geometry::intersection(geo, ToBox(box), piece);
  } catch (const boost::geometry::exception &) {
    /* self-intersecting geometries → skip this piece */
    return;
  }

  for (auto &polygon : piece)
    out.push_back(std::move(polygon));
}

/**
 * Clip the quadrilateral inside the screen bounds.
 *
 * Screen bounds that wrap the antimeridian are split into two ordinary
 * boxes (±180° seam); a single boost box cannot represent that wrap.
 */
[[gnu::pure]]
static ClippedMultiPolygon
Clip(const GeoQuadrilateral &_geo, const GeoBounds &_bounds) noexcept
{
  const auto geo = ToArrayQuadrilateral(_geo);
  ClippedMultiPolygon clipped;

  if (!LongitudeWraps(_bounds)) {
    ClipAgainstBox(geo, _bounds, clipped);
    return clipped;
  }

  /* west..+180° and -180°..east */
  const GeoBounds west_side(
    GeoPoint(_bounds.GetWest(), _bounds.GetNorth()),
    GeoPoint(Angle::HalfCircle(), _bounds.GetSouth()));
  const GeoBounds east_side(
    GeoPoint(-Angle::HalfCircle(), _bounds.GetNorth()),
    GeoPoint(_bounds.GetEast(), _bounds.GetSouth()));

  ClipAgainstBox(geo, west_side, clipped);
  ClipAgainstBox(geo, east_side, clipped);
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
  const bool flipped = bitmap.IsFlipped();

  /* Map a relative raster position (u,v in 0..1, from the top-left) to a
     texture coordinate. Accounts for the power-of-two padding-
     and image flipping. The result is clamped to the initialised
     region when padded. */
  const auto texcoord = [x_factor, y_factor, flipped](double u, double v) {
    double tx = u * x_factor;
    double ty = v * y_factor;
    if (flipped)
      ty = y_factor - ty;

    Point2D<GLfloat> c;
    c.x = std::clamp(tx, 0.0, double(x_factor));
    c.y = std::clamp(ty, 0.0, double(y_factor));
    return c;
  };

  texture.Bind();
  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);

  /* Curved-projection overlays carry a subdivision mesh. In that
     case, draw it cell by cell, each node textured with its exact
     texture coordinate and positioned by its own GeoToScreen(). */
  const auto draw_mesh = [&]() {
    Point2D<GLfloat> coord[4];
    BulkPixelPoint vertices[4]{};
    const ScopeVertexPointer vp(vertices);
    glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                          0, coord);

    const unsigned nx = grid.nx, ny = grid.ny;
    for (unsigned j = 0; j < ny; ++j) {
      for (unsigned i = 0; i < nx; ++i) {
        const GeoPoint tl = grid.At(i, j), tr = grid.At(i + 1, j);
        const GeoPoint bl = grid.At(i, j + 1), br = grid.At(i + 1, j + 1);

        /* skip cells that are entirely off-screen */
        GeoBounds cell = GeoBounds::Invalid();
        cell.Extend(tl);
        cell.Extend(tr);
        cell.Extend(bl);
        cell.Extend(br);
        if (!cell.Overlaps(screen_bounds))
          continue;

        const double u0 = double(i) / nx, u1 = double(i + 1) / nx;
        const double v0 = double(j) / ny, v1 = double(j + 1) / ny;

        /* GL_TRIANGLE_STRIP order: top-left, top-right, bottom-left,
           bottom-right */
        coord[0] = texcoord(u0, v0);
        vertices[0] = projection.GeoToScreen(tl);
        coord[1] = texcoord(u1, v0);
        vertices[1] = projection.GeoToScreen(tr);
        coord[2] = texcoord(u0, v1);
        vertices[2] = projection.GeoToScreen(bl);
        coord[3] = texcoord(u1, v1);
        vertices[3] = projection.GeoToScreen(br);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      }
    }
  };

  /* Large rasters without a georeference mesh: subdivide the corner
     quadrilateral, so the projection curvature is approximated at
     least piecewise. */
  const auto draw_sliced_quad = [&]() {
    Point2D<GLfloat> coord[4];
    BulkPixelPoint vertices[4]{};
    const ScopeVertexPointer vp(vertices);
    glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                          0, coord);

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
          coord[i] = texcoord(uv[i][0], uv[i][1]);
          vertices[i] = projection.GeoToScreen(geo[i]);
        }

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      }
    }
  };

  /* A single quadrilateral (flat rasters, e.g. weather overlays): clip to the
     screen and simply texture via the bilinear inverse */
  const auto draw_single_quad = [&]() {
    const auto clipped = Clip(bounds, screen_bounds);
    if (clipped.empty())
      return;

    Point2D<GLfloat> coord[16];
    BulkPixelPoint vertices[16];
    const ScopeVertexPointer vp(vertices);
    glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                          0, coord);

    for (const auto &polygon : clipped) {
      const auto &ring = polygon.outer();

      size_t n = ring.size();
      if (ring.front() == ring.back())
        --n;

      for (size_t i = 0; i < n; ++i) {
        const auto v = GeoFrom2D(ring[i]);
        const auto p = MapInQuadrilateral(bounds, v);
        coord[i] = texcoord(p.x, p.y);
        vertices[i] = projection.GeoToScreen(v);
      }

      glDrawArrays(GL_TRIANGLE_FAN, 0, n);
    }
  };

  const auto render = [&]() {
    if (grid.IsMesh())
      draw_mesh();
    else if (texture.GetWidth() > 512 || texture.GetHeight() > 512)
      draw_sliced_quad();
    else
      draw_single_quad();
  };

  if (blend_mode == MapOverlayBlendMode::ADD) {
    const ScopeTextureMultiplyAlpha blend(alpha);
    render();
  } else {
    const ScopeTextureConstantAlpha blend(use_bitmap_alpha, alpha);
    render();
  }

  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
}
