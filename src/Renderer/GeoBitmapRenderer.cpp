/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

  const ScopeVertexPointer vp(vertices);

  const GLTexture &texture = bitmap.BindAndGetTexture();
  const PixelSize allocated = texture.GetAllocatedSize();

  const GLfloat src_x = 0, src_y = 0, src_width = bitmap_size.width,
    src_height = bitmap_size.height;

  GLfloat x0 = src_x / allocated.width;
  GLfloat y0 = src_y / allocated.height;
  GLfloat x1 = (src_x + src_width) / allocated.width;
  GLfloat y1 = (src_y + src_height) / allocated.height;

  const GLfloat coord[] = {
    x0, y0,
    x1, y0,
    x0, y1,
    x1, y1,
  };

  OpenGL::texture_shader->Use();
  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, coord);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  OpenGL::solid_shader->Use();
}

#endif
