/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Screen/OpenGL/Compatibility.hpp"
#include "Projection/Projection.hpp"
#include "OS/Path.hpp"

MapOverlayBitmap::MapOverlayBitmap(Path path) throw(std::runtime_error)
{
  bounds = bitmap.LoadGeoFile(path);
}

void
MapOverlayBitmap::Draw(Canvas &canvas, const Projection &projection) noexcept
{
  const RasterPoint vertices[] = {
    projection.GeoToScreen(bounds.top_left),
    projection.GeoToScreen(bounds.top_right),
    projection.GeoToScreen(bounds.bottom_left),
    projection.GeoToScreen(bounds.bottom_right),
  };

  const ScopeVertexPointer vp(vertices);

  GLTexture &texture = *bitmap.GetNative();
  texture.Bind();

  const PixelSize allocated = texture.GetAllocatedSize();
  const unsigned src_x = 0, src_y = 0;
  const unsigned src_width = texture.GetWidth();
  const unsigned src_height = texture.GetHeight();

  GLfloat x0 = (GLfloat)src_x / allocated.cx;
  GLfloat y0 = (GLfloat)src_y / allocated.cy;
  GLfloat x1 = (GLfloat)(src_x + src_width) / allocated.cx;
  GLfloat y1 = (GLfloat)(src_y + src_height) / allocated.cy;

  if (bitmap.IsFlipped()) {
    y0 = 1 - y0;
    y1 = 1 - y1;
  }

  const GLfloat coord[] = {
    x0, y0,
    x1, y0,
    x0, y1,
    x1, y1,
  };

  const ScopeTextureConstantAlpha blend(alpha);

#ifdef USE_GLSL
  OpenGL::texture_shader->Use();
  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, coord);
#else
  const GLEnable<GL_TEXTURE_2D> scope;
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glTexCoordPointer(2, GL_FLOAT, 0, coord);
#endif

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifdef USE_GLSL
  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  OpenGL::solid_shader->Use();
#else
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
}
