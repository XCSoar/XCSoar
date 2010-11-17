/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Screen/Canvas.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Cache.hpp"
#include "Screen/Util.hpp"
#include "Math/FastMath.h"

#include <assert.h>

void
Canvas::fill_rectangle(int left, int top, int right, int bottom,
                       const Color color)
{
  color.set();

#ifdef ANDROID
  const GLvalue v[] = {
    left, top,
    right, top,
    right, bottom,
    left, bottom,
  };
  glVertexPointer(2, GL_VALUE, 0, v);

  GLubyte i[] = { 0, 1, 2, 0, 2, 3 };
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, i);
#else
  glRecti(left, top, right, bottom);
#endif
}

void
Canvas::polyline(const RasterPoint *lppt, unsigned cPoints)
{
  glVertexPointer(2, GL_VALUE, 0, lppt);

  pen.get_color().set();
  glDrawArrays(GL_LINE_STRIP, 0, cPoints);
}

void
Canvas::polygon(const RasterPoint *lppt, unsigned cPoints)
{
  if (brush.is_hollow() && !pen.defined())
    return;

  glVertexPointer(2, GL_VALUE, 0, lppt);

  if (!brush.is_hollow()) {
    brush.get_color().set();
#ifdef ANDROID
    // XXX
    glDrawArrays(GL_TRIANGLES, 0, cPoints / 3);
#else
    glDrawArrays(GL_POLYGON, 0, cPoints);
#endif
  }

  if (pen_over_brush()) {
    pen.get_color().set();
    glDrawArrays(GL_LINE_LOOP, 0, cPoints);
  }
}

void
Canvas::two_lines(int ax, int ay, int bx, int by, int cx, int cy)
{
  pen.get_color().set();

  const GLvalue v[] = { ax, ay, bx, by, cx, cy };
  glVertexPointer(2, GL_VALUE, 0, v);
  glDrawArrays(GL_LINE_STRIP, 0, 3);
}

void
Canvas::two_lines(const RasterPoint a, const RasterPoint b,
                  const RasterPoint c)
{
  pen.get_color().set();

  const RasterPoint v[] = { a, b, c };
  glVertexPointer(2, GL_VALUE, 0, v);
  glDrawArrays(GL_LINE_STRIP, 0, 3);
}

void
Canvas::circle(int x, int y, unsigned radius)
{
  enum { COUNT = 32 };
  RasterPoint v[1 + COUNT], *p = v;

  /* center (only needed for filling) */
  p->x = x;
  p->y = y;
  p += 2;

  for (unsigned i = 0; i < COUNT; ++i) {
    p->x = x + ICOSTABLE[i * 4096 / COUNT] * (int)radius / 1024.;
    p->y = y + ISINETABLE[i * 4096 / COUNT] * (int)radius / 1024.;
    ++p;
  }

  /* end point == start point (only needed for filling) */
  v[1] = p[-1];

  glVertexPointer(2, GL_VALUE, 0, v);

  if (!brush.is_hollow()) {
    brush.get_color().set();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 2 + COUNT);
  }

  if (pen_over_brush()) {
    pen.get_color().set();
    glDrawArrays(GL_LINE_LOOP, 2, COUNT);
  }
}

void
Canvas::segment(int x, int y, unsigned radius,
                Angle start, Angle end, bool horizon)
{
  ::Segment(*this, x, y, radius, start, end, horizon);
}

void
Canvas::text(int x, int y, const TCHAR *text)
{
  if (font == NULL)
    return;

  glColor4f(1.0, 1.0, 1.0, 1.0);

  if (background_mode == TRANSPARENT) {
    GLTexture *texture = TextCache::get(font, Color::WHITE, Color::BLACK, text);
    if (texture == NULL)
      return;

    GLLogicOp logic_op(GL_AND);
    GLEnable scope(GL_TEXTURE_2D);

    texture->bind();
    texture->draw(x_offset, y_offset, x, y);

    if (text_color != Color::BLACK) {
      GLTexture *texture = TextCache::get(font, Color::BLACK, text_color, text);
      if (texture == NULL)
        return;

      logic_op.set(GL_OR);
      texture->bind();
      texture->draw(x_offset, y_offset, x, y);
    }
  } else {
    GLTexture *texture = TextCache::get(font, background_color, text_color, text);
    if (texture == NULL)
      return;

    GLEnable scope(GL_TEXTURE_2D);
    texture->bind();
    texture->draw(x_offset, y_offset, x, y);
  }
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const GLTexture &texture,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  texture.draw(x_offset, y_offset,
               dest_x, dest_y, dest_width, dest_height,
               src_x, src_y, src_width, src_height);
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const GLTexture &texture)
{
  stretch(dest_x, dest_y, dest_width, dest_height,
          texture, 0, 0, texture.get_width(), texture.get_height());
}

void
Canvas::copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             const Bitmap &src, int src_x, int src_y)
{
  stretch(dest_x, dest_y, dest_width, dest_height,
          src, src_x, src_y, dest_width, dest_height);
}

void
Canvas::copy(const Bitmap &src)
{
  copy(0, 0, src.get_width(), src.get_height(), src, 0, 0);
}

void
Canvas::stretch_transparent(const Bitmap &src, Color key)
{
  assert(src.defined());

  // XXX
  stretch(src);
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src, int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  assert(src.defined());

  glColor4f(1.0, 1.0, 1.0, 1.0);

  GLTexture &texture = *src.native();
  GLEnable scope(GL_TEXTURE_2D);
  texture.bind();
  texture.draw(x_offset, y_offset,
               dest_x, dest_y, dest_width, dest_height,
               src_x, src_y, src_width, src_height);
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src)
{
  assert(src.defined());

  glColor4f(1.0, 1.0, 1.0, 1.0);

  GLTexture &texture = *src.native();
  GLEnable scope(GL_TEXTURE_2D);
  texture.bind();
  texture.draw(x_offset, y_offset,
               dest_x, dest_y, dest_width, dest_height,
               0, 0, src.get_width(), src.get_height());
}

void
Canvas::copy_or(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src, int src_x, int src_y)
{
  assert(src.defined());

  GLLogicOp logic_op(GL_OR);
  copy(dest_x, dest_y, dest_width, dest_height,
       src, src_x, src_y);
}

void
Canvas::copy_and(int dest_x, int dest_y,
                 unsigned dest_width, unsigned dest_height,
                 const Bitmap &src, int src_x, int src_y)
{
  assert(src.defined());

  GLLogicOp logic_op(GL_AND);
  copy(dest_x, dest_y, dest_width, dest_height,
       src, src_x, src_y);
}
