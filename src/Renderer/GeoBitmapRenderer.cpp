// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GeoBitmapRenderer.hpp"
#include "ui/canvas/RawBitmap.hpp"
#include "Geo/GeoBounds.hpp"
#include "Projection/Projection.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Texture.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#include "ui/canvas/opengl/VertexPointer.hpp"
#include "ui/canvas/opengl/Attribute.hpp"
#include "ui/dim/BulkPoint.hpp"

#include <algorithm>
#include <cmath>

static unsigned
GeoMeshDivisions(const GeoBounds &bounds) noexcept
{
  const double span = std::max(std::fabs(bounds.GetWidth().Degrees()),
                               std::fabs(bounds.GetHeight().Degrees()));
  if (!(span > 1.0))
    return 1;
  const unsigned d = 1 + (unsigned)span;
  return d > 16 ? 16 : d;
}

void
DrawGeoMesh(const GeoBounds &bounds, const Projection &projection,
            float u0, float v0, float u1, float v1)
{
  assert(bounds.IsValid());

  const unsigned div = GeoMeshDivisions(bounds);
  const Angle north = bounds.GetNorth();
  const Angle west = bounds.GetWest();
  const Angle dlat = bounds.GetHeight() / double(div);
  const Angle dlon = bounds.GetWidth() / double(div);
  const GLfloat du = (u1 - u0) / GLfloat(div);
  const GLfloat dv = (v1 - v0) / GLfloat(div);

  constexpr unsigned MAX_DIV = 16;
  BulkPixelPoint vertices[2 * (MAX_DIV + 1)];
  GLfloat coord[4 * (MAX_DIV + 1)];

  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);

  for (unsigned j = 0; j < div; ++j) {
    const Angle lat0 = north - dlat * double(j);
    const Angle lat1 = north - dlat * double(j + 1);
    const GLfloat vj0 = v0 + dv * GLfloat(j);
    const GLfloat vj1 = v0 + dv * GLfloat(j + 1);

    unsigned n = 0;
    for (unsigned i = 0; i <= div; ++i) {
      const Angle lon = west + dlon * double(i);
      const GLfloat u = u0 + du * GLfloat(i);
      vertices[n] = projection.GeoToScreen(GeoPoint(lon, lat0));
      coord[n * 2] = u;
      coord[n * 2 + 1] = vj0;
      ++n;
      vertices[n] = projection.GeoToScreen(GeoPoint(lon, lat1));
      coord[n * 2] = u;
      coord[n * 2 + 1] = vj1;
      ++n;
    }

    const ScopeVertexPointer vp(vertices);
    glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT,
                          GL_FALSE, 0, coord);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, n);
  }

  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
}

void
DrawGeoBitmap(const RawBitmap &bitmap, PixelSize bitmap_size,
              const GeoBounds &bounds,
              const Projection &projection)
{
  const GLTexture &texture = bitmap.BindAndGetTexture();
  const PixelSize allocated = texture.GetAllocatedSize();

  const GLfloat x0 = 0;
  const GLfloat y0 = 0;
  const GLfloat x1 = GLfloat(bitmap_size.width) / allocated.width;
  const GLfloat y1 = GLfloat(bitmap_size.height) / allocated.height;

  DrawGeoMesh(bounds, projection, x0, y0, x1, y1);
}

#endif
