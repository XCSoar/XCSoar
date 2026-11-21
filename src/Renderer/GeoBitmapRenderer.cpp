// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GeoBitmapRenderer.hpp"
#include "ui/canvas/RawBitmap.hpp"
#include "Geo/GeoBounds.hpp"
#include "Projection/Projection.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Texture.hpp"
#include "ui/canvas/opengl/VertexPointer.hpp"
#include "ui/canvas/opengl/Attribute.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "Geo/GeoPoint.hpp"

#include <algorithm>
#include <cmath>

/** One strip per pixel of north/south edge mismatch, capped. */
static constexpr unsigned MAX_GEO_QUAD_STRIPS = 160;

/**
 * Screen length of an edge, in pixels.
 */
static int
ScreenSpan(PixelPoint a, PixelPoint b) noexcept
{
  const double dx = double(a.x - b.x);
  const double dy = double(a.y - b.y);
  return int(std::hypot(dx, dy));
}

/**
 * How many latitude strips keep the longitude error under a pixel.
 * The error of one quad is about a quarter of the difference between
 * the north and south edge lengths.
 */
static unsigned
GeoQuadStrips(const Projection &projection,
              const GeoBounds &bounds) noexcept
{
  const PixelPoint nw = projection.GeoToScreen(bounds.GetNorthWest());
  const PixelPoint ne = projection.GeoToScreen(bounds.GetNorthEast());
  const PixelPoint sw = projection.GeoToScreen(bounds.GetSouthWest());
  const PixelPoint se = projection.GeoToScreen(bounds.GetSouthEast());
  const int skew = std::abs(ScreenSpan(nw, ne) - ScreenSpan(sw, se));
  if (skew <= 1)
    return 1;

  return std::min(unsigned(skew), MAX_GEO_QUAD_STRIPS);
}

void
DrawGeoQuad(const GeoBounds &bounds, const Projection &projection,
            float u1, float v1) noexcept
{
  assert(bounds.IsValid());

  const unsigned strips = GeoQuadStrips(projection, bounds);
  const unsigned n = (strips + 1) * 2;

  BulkPixelPoint vertices[(MAX_GEO_QUAD_STRIPS + 1) * 2];
  GLfloat coord[(MAX_GEO_QUAD_STRIPS + 1) * 4];

  const Angle north = bounds.GetNorth();
  const Angle west = bounds.GetWest();
  const Angle east = bounds.GetEast();
  const Angle step = bounds.GetHeight() / double(strips);

  for (unsigned i = 0; i <= strips; ++i) {
    const Angle lat = north - step * double(i);
    vertices[i * 2] = projection.GeoToScreen(GeoPoint(west, lat));
    vertices[i * 2 + 1] = projection.GeoToScreen(GeoPoint(east, lat));

    const GLfloat v = v1 * GLfloat(i) / GLfloat(strips);
    coord[i * 4] = 0;
    coord[i * 4 + 1] = v;
    coord[i * 4 + 2] = u1;
    coord[i * 4 + 3] = v;
  }

  const ScopeVertexPointer vp(vertices);
  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, coord);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, GLsizei(n));
  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
}

/**
 * Draw a geo-referenced bitmap texture.  The caller is responsible
 * for setting up the OpenGL shader (e.g. via
 * ScopeTextureConstantAlpha) before calling this function.
 */
void
DrawGeoBitmap(const RawBitmap &bitmap, PixelSize bitmap_size,
              const GeoBounds &bounds,
              const Projection &projection)
{
  assert(bounds.IsValid());

  const GLTexture &texture = bitmap.BindAndGetTexture();
  const PixelSize allocated = texture.GetAllocatedSize();

  const GLfloat x1 = GLfloat(bitmap_size.width) / allocated.width;
  const GLfloat y1 = GLfloat(bitmap_size.height) / allocated.height;
  DrawGeoQuad(bounds, projection, x1, y1);
}

#endif
