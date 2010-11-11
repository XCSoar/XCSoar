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
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/Util.hpp"
#include "Math/FastMath.h"

#include <assert.h>

void
Canvas::fill_rectangle(int left, int top, int right, int bottom,
                       const Color color)
{
  color.set();

#ifdef ANDROID
  const GLfloat v[] = {
    left, top,
    right, top,
    right, bottom,
    left, bottom,
  };
  glVertexPointer(2, GL_FLOAT, 0, v);

  GLubyte i[] = { 0, 1, 2, 0, 2, 3 };
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, i);
#else
  glRecti(left, top, right, bottom);
#endif
}

static void
PointsToVertices(GLfloat *dest, const POINT *src, unsigned count)
{
  while (count-- > 0) {
    *dest++ = src->x;
    *dest++ = src->y;
    ++src;
  }
}

void
Canvas::polyline(const POINT *lppt, unsigned cPoints)
{
  GLfloat v[cPoints * 2];
  PointsToVertices(v, lppt, cPoints);
  glVertexPointer(2, GL_FLOAT, 0, v);

  pen.get_color().set();
  glDrawArrays(GL_LINE_STRIP, 0, cPoints);
}

void
Canvas::polygon(const POINT* lppt, unsigned cPoints)
{
  if (brush.is_hollow() && !pen.defined())
    return;

  GLfloat v[cPoints * 2];
  PointsToVertices(v, lppt, cPoints);
  glVertexPointer(2, GL_FLOAT, 0, v);

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
Canvas::circle(int x, int y, unsigned radius)
{
  enum { COUNT = 32 };
  GLfloat v[(2 + COUNT) * 2], *p = v;

  /* center (only needed for filling) */
  *p++ = x;
  *p++ = y;
  p += 2;

  for (unsigned i = 0; i < COUNT; ++i) {
    *p++ = x + ICOSTABLE[i * 4096 / COUNT] * (int)radius / 1024.;
    *p++ = y + ISINETABLE[i * 4096 / COUNT] * (int)radius / 1024.;
  }

  /* end point == start point (only needed for filling) */
  v[2] = p[-2];
  v[3] = p[-1];

  glVertexPointer(2, GL_FLOAT, 0, v);

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
  SDL_Surface *s;

  if (font == NULL)
    return;

  s = ::TTF_RenderUTF8_Solid(font, text, Color::BLACK);
  if (s == NULL)
    return;

  if (background_mode == TRANSPARENT && s->format->palette != NULL &&
      s->format->palette->ncolors >= 2) {
    s->flags &= ~SDL_SRCCOLORKEY;

    GLLogicOp logic_op(GL_AND);

    /* clear the text pixels (AND) */
    s->format->palette->colors[0] = Color::WHITE;
    s->format->palette->colors[1] = Color::BLACK;
    copy(x, y, s);

    /* paint with the text color on top (OR) */
    if (text_color != Color::BLACK) {
      s->format->palette->colors[0] = Color::BLACK;
      s->format->palette->colors[1] = text_color;
      logic_op.set(GL_OR);
      copy(x, y, s);
    }

    ::SDL_FreeSurface(s);
    return;
  }

  if (s->format->palette != NULL && s->format->palette->ncolors >= 2) {
    s->format->palette->colors[1] = text_color;

    if (background_mode == OPAQUE) {
      s->flags &= ~SDL_SRCCOLORKEY;
      s->format->palette->colors[0] = background_color;
    }
  }

  copy(x, y, s);
  ::SDL_FreeSurface(s);
}

void
Canvas::copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             SDL_Surface *src_surface, int src_x, int src_y)
{
  assert(src_surface != NULL);

  glColor4f(1.0, 1.0, 1.0, 1.0);

  GLTexture texture(src_surface);
  texture.draw(x_offset, y_offset,
               dest_x, dest_y, dest_width, dest_height,
               src_x, src_y, dest_width, dest_height,
               src_surface->w, src_surface->h);
}

void
Canvas::stretch_transparent(const Canvas &src, Color key)
{
  assert(src.surface != NULL);

  // XXX
  stretch(src);
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  assert(src.surface != NULL);

  glColor4f(1.0, 1.0, 1.0, 1.0);

  GLTexture texture(src.surface);
  texture.draw(x_offset, y_offset,
               dest_x, dest_y, dest_width, dest_height,
               src_x, src_y, src_width, src_height,
               src.surface->w, src.surface->h);
}

void
Canvas::copy_or(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src, int src_x, int src_y)
{
  assert(src.surface != NULL);

  stretch_or(dest_x, dest_y, dest_width, dest_height,
             src, src_x, src_y, dest_width, dest_height);
}

void
Canvas::copy_and(int dest_x, int dest_y,
                 unsigned dest_width, unsigned dest_height,
                 const Canvas &src, int src_x, int src_y)
{
  assert(src.surface != NULL);

  stretch_and(dest_x, dest_y, dest_width, dest_height,
                src, src_x, src_y, dest_width, dest_height);
}

void
Canvas::stretch_or(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   const Canvas &src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height)
{
  assert(src.surface != NULL);

  GLLogicOp logic_op(GL_OR);
  stretch(dest_x, dest_y, dest_width, dest_height,
          src, src_x, src_y, src_width, src_height);
}

void
Canvas::stretch_and(int dest_x, int dest_y,
                    unsigned dest_width, unsigned dest_height,
                    const Canvas &src,
                    int src_x, int src_y,
                    unsigned src_width, unsigned src_height)
{
  assert(src.surface != NULL);

  GLLogicOp logic_op(GL_AND);
  stretch(dest_x, dest_y, dest_width, dest_height,
          src, src_x, src_y, src_width, src_height);
}
