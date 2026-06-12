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
#include "ui/canvas/opengl/Shaders.hpp"
#include "ui/canvas/opengl/Program.hpp"
#include "ui/canvas/opengl/DitherPass.hpp"
#include "ui/dim/BulkPoint.hpp"

void
DrawGeoBitmap(const RawBitmap &bitmap, PixelSize bitmap_size,
              const GeoBounds &bounds,
              const Projection &projection)
{
  assert(bounds.IsValid());

  const BulkPixelPoint vertices[] = {
    projection.GeoToScreen(bounds.GetNorthWest()),
    projection.GeoToScreen(bounds.GetNorthEast()),
    projection.GeoToScreen(bounds.GetSouthWest()),
    projection.GeoToScreen(bounds.GetSouthEast()),
  };

  const GLTexture &texture = bitmap.BindAndGetTexture();
  const PixelSize allocated = texture.GetAllocatedSize();

  const GLfloat src_x = 0, src_y = 0, src_width = bitmap_size.width,
    src_height = bitmap_size.height;

  const GLfloat texcoord[] = {
    src_x / allocated.width, src_y / allocated.height,
    (src_x + src_width) / allocated.width, src_y / allocated.height,
    src_x / allocated.width, (src_y + src_height) / allocated.height,
    (src_x + src_width) / allocated.width, (src_y + src_height) / allocated.height,
  };

  if (OpenGL::enable_dither_pass &&
      OpenGL::dither_algorithm != OpenGL::DitherAlgorithm::NONE)
    OpenGL::DrawGeoBitmapWithDither(texture, bitmap_size, vertices, texcoord);
  else {
    const ScopeVertexPointer vp(vertices);

    OpenGL::texture_shader->Use();

    glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
    glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                          0, texcoord);

    const_cast<GLTexture &>(texture).Bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  }
}

#endif
