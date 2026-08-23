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
#include "Math/Boost/Point.hpp"
#include "system/Path.hpp"
#include "util/StaticArray.hxx"

#include <algorithm>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/strategies/strategies.hpp>

using ArrayQuadrilateral = StaticArray<DoublePoint2D, 5>;
BOOST_GEOMETRY_REGISTER_RING(ArrayQuadrilateral);

MapOverlayBitmap::MapOverlayBitmap(Path path)
  :label((path.GetBase() != nullptr ? path.GetBase() : path).c_str())
{
  bounds = bitmap.LoadGeoFile(path);
  simple_bounds = bounds.GetBounds();
}

/**
 * Convert a GeoPoint to a "fake" flat DoublePoint2D.
 */
static constexpr DoublePoint2D
GeoTo2D(GeoPoint p) noexcept
{
  return {p.longitude.Native(), p.latitude.Native()};
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

  Point2D<GLfloat> coord[4];
  /* Float positions so OpenGL can clip corners that sit far off the
     screen.  GLshort wraps at ±32767 and the tile vanishes. */
  FloatPoint2D vertices[4];

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
          coord[i].x = GLfloat(uv[i][0] * x_factor);
          coord[i].y = GLfloat((bitmap.IsFlipped() ? 1 - uv[i][1]
                                                   : uv[i][1]) * y_factor);
          const auto sp = projection.GeoToScreen(geo[i]);
          vertices[i] = FloatPoint2D(float(sp.x), float(sp.y));
        }

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      }
    }

    glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
    return;
  }

  /* Draw the full quad and let OpenGL clip.  Clipping to the screen
     AABB then rebuilding a TRIANGLE_FAN made partly visible tiles
     flicker (MapInQuadrilateral UVs jumped to -1 on the clip edge). */
  const GeoPoint geo[4] = {
    bounds.top_left,
    bounds.top_right,
    bounds.bottom_right,
    bounds.bottom_left,
  };
  const double uv[4][2] = {
    {0, 0},
    {1, 0},
    {1, 1},
    {0, 1},
  };

  for (unsigned i = 0; i < 4; ++i) {
    coord[i].x = GLfloat(uv[i][0] * x_factor);
    coord[i].y = GLfloat((bitmap.IsFlipped() ? 1 - uv[i][1]
                                             : uv[i][1]) * y_factor);
    const auto sp = projection.GeoToScreen(geo[i]);
    vertices[i] = FloatPoint2D(float(sp.x), float(sp.y));
  }

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
}
