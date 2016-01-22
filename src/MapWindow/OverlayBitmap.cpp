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

#include "OverlayBitmap.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/ConstantAlpha.hpp"
#include "Screen/OpenGL/VertexPointer.hpp"
#include "Projection/WindowProjection.hpp"
#include "Math/Point2D.hpp"
#include "Math/Quadrilateral.hpp"
#include "Math/Boost/Point.hpp"
#include "OS/Path.hpp"
#include "Util/StaticArray.hxx"

#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/strategies/strategies.hpp>

using ArrayQuadrilateral = StaticArray<DoublePoint2D, 5>;
BOOST_GEOMETRY_REGISTER_RING(ArrayQuadrilateral);

using ClippedPolygon = boost::geometry::model::polygon<DoublePoint2D>;

using ClippedMultiPolygon =
  boost::geometry::model::multi_polygon<ClippedPolygon>;

MapOverlayBitmap::MapOverlayBitmap(Path path) throw(std::runtime_error)
  :label((path.GetBase() != nullptr ? path.GetBase() : path).c_str())
{
  bounds = bitmap.LoadGeoFile(path);
  simple_bounds = bounds.GetBounds();
}

/**
 * Convert a GeoPoint to a "fake" flat DoublePoint2D.  This conversion
 * is flawed in many ways, but good enough for clipping polygons.
 */
static inline constexpr DoublePoint2D
GeoTo2D(GeoPoint p)
{
  return {p.longitude.Native(), p.latitude.Native()};
}

/**
 * Inverse of GeoTo2D().
 */
static inline constexpr GeoPoint
GeoFrom2D(DoublePoint2D p)
{
  return {Angle::Native(p.x), Angle::Native(p.y)};
}

/**
 * Convert a #GeoBounds instance to a boost::geometry box.
 */
gcc_const
static boost::geometry::model::box<DoublePoint2D>
ToBox(const GeoBounds b)
{
  return {GeoTo2D(b.GetSouthWest()), GeoTo2D(b.GetNorthEast())};
}

/**
 * Convert a #GeoQuadrilateral instance to a boost::geometry ring.
 */
gcc_const
static ArrayQuadrilateral
ToArrayQuadrilateral(const GeoQuadrilateral q)
{
  return {GeoTo2D(q.top_left), GeoTo2D(q.top_right),
      GeoTo2D(q.bottom_right), GeoTo2D(q.bottom_left),
      /* close the ring: */
      GeoTo2D(q.top_left) };
}

/**
 * Clip the quadrilateral inside the screen bounds.
 */
gcc_pure
static ClippedMultiPolygon
Clip(const GeoQuadrilateral &_geo, const GeoBounds &_bounds)
{
  const auto geo = ToArrayQuadrilateral(_geo);
  const auto bounds = ToBox(_bounds);

  ClippedMultiPolygon clipped;
  boost::geometry::intersection(geo, bounds, clipped);
  return clipped;
}

gcc_pure
static DoublePoint2D
MapInQuadrilateral(const GeoQuadrilateral &q, const GeoPoint p)
{
  return MapInQuadrilateral(GeoTo2D(q.top_left), GeoTo2D(q.top_right),
                            GeoTo2D(q.bottom_right), GeoTo2D(q.bottom_left),
                            GeoTo2D(p));
}

bool
MapOverlayBitmap::IsInside(GeoPoint p) const
{
  return simple_bounds.IsInside(p) &&
    boost::geometry::covered_by(GeoTo2D(p), ToArrayQuadrilateral(bounds));
}

void
MapOverlayBitmap::Draw(Canvas &canvas,
                       const WindowProjection &projection) noexcept
{
  if (!simple_bounds.Overlaps(projection.GetScreenBounds()))
    /* not visible, outside of screen area */
    return;

  auto clipped = Clip(bounds, projection.GetScreenBounds());
  if (clipped.empty())
    return;

  GLTexture &texture = *bitmap.GetNative();
  const PixelSize allocated = texture.GetAllocatedSize();
  const double x_factor = double(texture.GetWidth()) / allocated.cx;
  const double y_factor = double(texture.GetHeight()) / allocated.cy;

  Point2D<GLfloat> coord[16];
  BulkPixelPoint vertices[16];

  const ScopeVertexPointer vp(vertices);

  texture.Bind();

  const ScopeTextureConstantAlpha blend(use_bitmap_alpha, alpha);

#ifdef USE_GLSL
  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, coord);
#else
  const GLEnable<GL_TEXTURE_2D> scope;
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glTexCoordPointer(2, GL_FLOAT, 0, coord);
#endif

  for (const auto &polygon : clipped) {
    const auto &ring = polygon.outer();

    size_t n = ring.size();
    if (ring.front() == ring.back())
      --n;

    for (size_t i = 0; i < n; ++i) {
      const auto v = GeoFrom2D(ring[i]);

      auto p = MapInQuadrilateral(bounds, v);
      coord[i].x = p.x * x_factor;
      coord[i].y = p.y * y_factor;

      if (bitmap.IsFlipped())
        coord[i].y = 1 - coord[i].y;

      vertices[i] = projection.GeoToScreen(v);
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, n);
  }

#ifdef USE_GLSL
  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
#else
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
}
