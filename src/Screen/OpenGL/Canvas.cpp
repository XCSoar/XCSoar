/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Cache.hpp"
#include "Screen/OpenGL/VertexArray.hpp"
#include "Screen/OpenGL/Shapes.hpp"
#include "Screen/OpenGL/Buffer.hpp"
#include "Screen/OpenGL/Features.hpp"
#include "Screen/OpenGL/Compatibility.hpp"
#include "Screen/Util.hpp"

#include <assert.h>

AllocatedArray<RasterPoint> Canvas::vertex_buffer;

void
Canvas::fill_rectangle(int left, int top, int right, int bottom,
                       const Color color)
{
  color.set();

#ifdef HAVE_GLES
  const RasterPoint vertices[] = {
    { left, top },
    { right, top },
    { left, bottom },
    { right, bottom },
  };

  glVertexPointer(2, GL_VALUE, 0, vertices);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#else
  glRecti(left, top, right, bottom);
#endif
}

void
Canvas::OutlineRectangleGL(int left, int top, int right, int bottom)
{
  const RasterPoint vertices[] = {
    { left, top },
    { right, top },
    { right, bottom },
    { left, bottom },
  };

  glVertexPointer(2, GL_VALUE, 0, vertices);
  glDrawArrays(GL_LINE_LOOP, 0, 4);
}

void
Canvas::raised_edge(PixelRect &rc)
{
  Pen bright(1, Color(240, 240, 240));
  select(bright);
  two_lines(rc.left, rc.bottom - 2, rc.left, rc.top,
            rc.right - 2, rc.top);

  Pen dark(1, Color(128, 128, 128));
  select(dark);
  two_lines(rc.left + 1, rc.bottom - 1, rc.right - 1, rc.bottom - 1,
            rc.right - 1, rc.top + 1);

  ++rc.left;
  ++rc.top;
  --rc.right;
  --rc.bottom;
}

void
Canvas::polyline(const RasterPoint *points, unsigned num_points)
{
  glVertexPointer(2, GL_VALUE, 0, points);

  pen.set();
  glDrawArrays(GL_LINE_STRIP, 0, num_points);
}

void
Canvas::polygon(const RasterPoint *points, unsigned num_points)
{
  if (brush.is_hollow() && !pen.defined())
    return;

  glVertexPointer(2, GL_VALUE, 0, points);

  if (!brush.is_hollow() && num_points >= 3) {
    brush.set();

    static AllocatedArray<GLushort> triangle_buffer;
    triangle_buffer.grow_discard(3 * (num_points - 2));
    unsigned idx_count = polygon_to_triangle(points, num_points,
                                             triangle_buffer.begin());
    if (idx_count > 0)
      glDrawElements(GL_TRIANGLES, idx_count, GL_UNSIGNED_SHORT,
                     triangle_buffer.begin());
  }

  if (pen_over_brush()) {
    pen.set();
    if (pen.get_width() <= 2) {
      glDrawArrays(GL_LINE_LOOP, 0, num_points);
    } else {
      vertex_buffer.grow_discard(2 * (num_points + 1));
      unsigned vertices = line_to_triangle(points, num_points,
                                           vertex_buffer.begin(),
                                           pen.get_width(), true);
      if (vertices > 0) {
        glVertexPointer(2, GL_VALUE, 0, vertex_buffer.begin());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices);
      }
    }
  }
}

void
Canvas::TriangleFan(const RasterPoint *points, unsigned num_points)
{
  if (brush.is_hollow() && !pen.defined())
    return;

  glVertexPointer(2, GL_VALUE, 0, points);

  if (!brush.is_hollow() && num_points >= 3) {
    brush.set();
    glDrawArrays(GL_TRIANGLE_FAN, 0, num_points);
  }

  if (pen_over_brush()) {
    pen.set();
    if (pen.get_width() <= 2) {
      glDrawArrays(GL_LINE_LOOP, 0, num_points);
    } else {
      vertex_buffer.grow_discard(2 * (num_points + 1));
      unsigned vertices = line_to_triangle(points, num_points,
                                           vertex_buffer.begin(),
                                           pen.get_width(), true);
      if (vertices > 0) {
        glVertexPointer(2, GL_VALUE, 0, vertex_buffer.begin());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices);
      }
    }
  }
}

void
Canvas::line(int ax, int ay, int bx, int by)
{
  pen.set();

  const GLvalue v[] = { ax, ay, bx, by };
  glVertexPointer(2, GL_VALUE, 0, v);
  glDrawArrays(GL_LINE_STRIP, 0, 2);
}

/**
 * Draw a line from a to b, using triangle caps if pen-size > 2 to hide
 * gaps between consecutive lines.
 */
void
Canvas::line_piece(const RasterPoint a, const RasterPoint b)
{
  pen.set();

  const RasterPoint v[] = { {a.x, a.y}, {b.x, b.y} };
  if (pen.get_width() > 2) {
    RasterPoint strip[6];
    unsigned strip_len = line_to_triangle(v, 2, strip, pen.get_width(),
                                          false, true);
    if (strip_len > 0) {
      glVertexPointer(2, GL_VALUE, 0, &strip[0].x);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, strip_len);
    }
  } else {
    glVertexPointer(2, GL_VALUE, 0, &v[0].x);
    glDrawArrays(GL_LINE_STRIP, 0, 2);
  }
}

void
Canvas::two_lines(int ax, int ay, int bx, int by, int cx, int cy)
{
  pen.set();

  const GLvalue v[] = { ax, ay, bx, by, cx, cy };
  glVertexPointer(2, GL_VALUE, 0, v);
  glDrawArrays(GL_LINE_STRIP, 0, 3);
}

void
Canvas::two_lines(const RasterPoint a, const RasterPoint b,
                  const RasterPoint c)
{
  pen.set();

  const RasterPoint v[] = { a, b, c };
  glVertexPointer(2, GL_VALUE, 0, v);
  glDrawArrays(GL_LINE_STRIP, 0, 3);
}

void
Canvas::circle(int x, int y, unsigned radius)
{
  if (pen_over_brush() && pen.get_width() > 2) {
    GLDonutVertices vertices(x, y,
                             radius - pen.get_width()/2,
                             radius + pen.get_width()/2);
    if (!brush.is_hollow()) {
      vertices.bind_inner_circle();
      brush.set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.CIRCLE_SIZE);
    }
    vertices.bind();
    pen.set();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.SIZE);
  } else if (OpenGL::vertex_buffer_object && radius < 16) {
    /* draw a "small" circle with VBO */

    OpenGL::small_circle_buffer->Bind();
    glVertexPointer(2, GL_VALUE, 0, NULL);

    glPushMatrix();

#ifdef HAVE_GLES
    glTranslatex((GLfixed)x << 16, (GLfixed)y << 16, 0);
    glScalex((GLfixed)radius << 8, (GLfixed)radius << 8, (GLfixed)1 << 16);
#else
    glTranslatef(x, y, 0.);
    glScalef(radius / 256., radius / 256., 1.);
#endif

    if (!brush.is_hollow()) {
      brush.set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, OpenGL::SMALL_CIRCLE_SIZE);
    }

    if (pen_over_brush()) {
      pen.set();
      glDrawArrays(GL_LINE_LOOP, 0, OpenGL::SMALL_CIRCLE_SIZE);
    }

    glPopMatrix();

    OpenGL::small_circle_buffer->Unbind();
  } else if (OpenGL::vertex_buffer_object) {
    /* draw a "big" circle with VBO */

    OpenGL::circle_buffer->Bind();
    glVertexPointer(2, GL_VALUE, 0, NULL);

    glPushMatrix();

#ifdef HAVE_GLES
    glTranslatex((GLfixed)x << 16, (GLfixed)y << 16, 0);
    glScalex((GLfixed)radius << 6, (GLfixed)radius << 6, (GLfixed)1 << 16);
#else
    glTranslatef(x, y, 0.);
    glScalef(radius / 1024., radius / 1024., 1.);
#endif

    if (!brush.is_hollow()) {
      brush.set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, OpenGL::CIRCLE_SIZE);
    }

    if (pen_over_brush()) {
      pen.set();
      glDrawArrays(GL_LINE_LOOP, 0, OpenGL::CIRCLE_SIZE);
    }

    glPopMatrix();

    OpenGL::circle_buffer->Unbind();
  } else {
    GLCircleVertices vertices(x, y, radius);
    vertices.bind();

    if (!brush.is_hollow()) {
      brush.set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.SIZE);
    }

    if (pen_over_brush()) {
      pen.set();
      glDrawArrays(GL_LINE_LOOP, 0, vertices.SIZE);
    }
  }
}

void
Canvas::segment(int x, int y, unsigned radius,
                Angle start, Angle end, bool horizon)
{
  ::Segment(*this, x, y, radius, start, end, horizon);
}

gcc_const
static unsigned
AngleToDonutVertex(Angle angle)
{
  return (NATIVE_TO_INT(angle.value_native()) * 64 / 4096 + 48) & 0x3e;
}

void
Canvas::annulus(int x, int y, unsigned small_radius, unsigned big_radius,
                Angle start, Angle end)
{
  GLDonutVertices vertices(x, y, small_radius, big_radius);

  const unsigned istart = AngleToDonutVertex(start);
  const unsigned iend = AngleToDonutVertex(end);

  if (!brush.is_hollow()) {
    brush.set();
    vertices.bind();

    if (istart > iend) {
      glDrawArrays(GL_TRIANGLE_STRIP, istart, 64 - istart + 2);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, iend + 2);
    } else {
      glDrawArrays(GL_TRIANGLE_STRIP, istart, iend - istart + 2);
    }
  }

  if (pen_over_brush()) {
    pen.set();

    if (istart != iend) {
      if (brush.is_hollow())
        vertices.bind();

      glDrawArrays(GL_LINE_STRIP, istart, 2);
      glDrawArrays(GL_LINE_STRIP, iend, 2);
    }

    const unsigned pstart = istart / 2;
    const unsigned pend = iend / 2;

    vertices.bind_inner_circle();
    if (pstart < pend) {
      glDrawArrays(GL_LINE_STRIP, pstart, pend - pstart + 1);
    } else {
      glDrawArrays(GL_LINE_STRIP, pstart, 32 - pstart + 1);
      glDrawArrays(GL_LINE_STRIP, 0, pend + 1);
    }

    vertices.bind_outer_circle();
    if (pstart < pend) {
      glDrawArrays(GL_LINE_STRIP, pstart, pend - pstart + 1);
    } else {
      glDrawArrays(GL_LINE_STRIP, pstart, 32 - pstart + 1);
      glDrawArrays(GL_LINE_STRIP, 0, pend + 1);
    }
  }
}

void
Canvas::keyhole(int x, int y, unsigned small_radius, unsigned big_radius,
                  Angle start, Angle end)
{
  ::KeyHole(*this, x, y, big_radius, start, end, small_radius);
}

void
Canvas::draw_focus(PixelRect rc)
{
  outline_rectangle(rc.left, rc.top, rc.right, rc.bottom, COLOR_DARK_GRAY);
}

void
Canvas::text(int x, int y, const TCHAR *text)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  if (font == NULL)
    return;

  GLTexture *texture = TextCache::get(font, COLOR_BLACK, COLOR_WHITE, text);
  if (texture == NULL)
    return;

  if (background_mode == OPAQUE)
    /* draw the opaque background */
    fill_rectangle(x, y, x + texture->get_width(), y + texture->get_height(),
                   background_color);

  GLEnable scope(GL_TEXTURE_2D);
  texture->bind();
  GLLogicOp logic_op(GL_AND_INVERTED);

  if (background_mode != OPAQUE || background_color != COLOR_BLACK) {
    /* cut out the shape in black */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    texture->draw(x, y);
  }

  if (text_color != COLOR_BLACK) {
    /* draw the text color on top */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    logic_op.set(GL_OR);
    text_color.set();
    texture->draw(x, y);
  }
}

void
Canvas::text_transparent(int x, int y, const TCHAR *text)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  if (font == NULL)
    return;

  GLTexture *texture = TextCache::get(font, COLOR_BLACK, COLOR_WHITE, text);
  if (texture == NULL)
    return;

  GLEnable scope(GL_TEXTURE_2D);
  texture->bind();
  GLLogicOp logic_op(GL_AND_INVERTED);

  /* cut out the shape in black */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  texture->draw(x, y);

  if (text_color != COLOR_BLACK) {
    /* draw the text color on top */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    logic_op.set(GL_OR);
    text_color.set();
    texture->draw(x, y);
  }
}

void
Canvas::text_clipped(int x, int y, unsigned width, const TCHAR *text)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  if (font == NULL)
    return;

  GLTexture *texture = TextCache::get(font, COLOR_BLACK, COLOR_WHITE, text);
  if (texture == NULL)
    return;

  GLEnable scope(GL_TEXTURE_2D);
  texture->bind();
  GLLogicOp logic_op(GL_AND_INVERTED);

  unsigned height = texture->get_height();
  if (texture->get_width() < width)
    width = texture->get_width();

  /* cut out the shape in black */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  texture->draw(x, y, width, height, 0, 0, width, height);

  if (text_color != COLOR_BLACK) {
    /* draw the text color on top */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    logic_op.set(GL_OR);
    text_color.set();
    texture->draw(x, y, width, height, 0, 0, width, height);
  }
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const GLTexture &texture,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  texture.draw(dest_x, dest_y, dest_width, dest_height,
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
Canvas::invert_stretch_transparent(const Bitmap &src, Color key)
{
  assert(src.defined());

  // XXX
  GLLogicOp invert(GL_COPY_INVERTED);
  stretch(src);
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src, int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif
  assert(src.defined());

  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  GLTexture &texture = *src.native();
  GLEnable scope(GL_TEXTURE_2D);
  texture.bind();
  texture.draw(dest_x, dest_y, dest_width, dest_height,
               src_x, src_y, src_width, src_height);
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif
  assert(src.defined());

  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  GLTexture &texture = *src.native();
  GLEnable scope(GL_TEXTURE_2D);
  texture.bind();
  texture.draw(dest_x, dest_y, dest_width, dest_height,
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

void
Canvas::copy_not(int dest_x, int dest_y,
                 unsigned dest_width, unsigned dest_height,
                 const Bitmap &src, int src_x, int src_y)
{
  assert(src.defined());

  GLLogicOp logic_op(GL_COPY_INVERTED);
  copy(dest_x, dest_y, dest_width, dest_height,
       src, src_x, src_y);
}
