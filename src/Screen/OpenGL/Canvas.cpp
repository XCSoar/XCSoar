/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Util/Macros.hpp"

#ifndef NDEBUG
#include "Util/UTF8.hpp"
#endif

#include <assert.h>

AllocatedArray<RasterPoint> Canvas::vertex_buffer;

void
Canvas::DrawFilledRectangle(PixelScalar left, PixelScalar top,
                       PixelScalar right, PixelScalar bottom,
                       const Color color)
{
  color.Set();

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
Canvas::OutlineRectangleGL(PixelScalar left, PixelScalar top,
                           PixelScalar right, PixelScalar bottom)
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
Canvas::FadeToWhite(GLubyte alpha)
{
  const GLEnable blend(GL_BLEND);
  const Color color(0xff, 0xff, 0xff, alpha);
  Clear(color);
}

void
Canvas::FadeToWhite(PixelRect rc, GLubyte alpha)
{
  const GLEnable blend(GL_BLEND);
  const Color color(0xff, 0xff, 0xff, alpha);
  DrawFilledRectangle(rc.left, rc.right, rc.right, rc.bottom, color);
}

void
Canvas::DrawRaisedEdge(PixelRect &rc)
{
  Pen bright(1, Color(240, 240, 240));
  Select(bright);
  DrawTwoLines(rc.left, rc.bottom - 2, rc.left, rc.top,
            rc.right - 2, rc.top);

  Pen dark(1, Color(128, 128, 128));
  Select(dark);
  DrawTwoLines(rc.left + 1, rc.bottom - 1, rc.right - 1, rc.bottom - 1,
            rc.right - 1, rc.top + 1);

  ++rc.left;
  ++rc.top;
  --rc.right;
  --rc.bottom;
}

void
Canvas::DrawPolyline(const RasterPoint *points, unsigned num_points)
{
  glVertexPointer(2, GL_VALUE, 0, points);

  pen.Bind();
  glDrawArrays(GL_LINE_STRIP, 0, num_points);
  pen.Unbind();
}

void
Canvas::DrawPolygon(const RasterPoint *points, unsigned num_points)
{
  if (brush.IsHollow() && !pen.IsDefined())
    return;

  glVertexPointer(2, GL_VALUE, 0, points);

  if (!brush.IsHollow() && num_points >= 3) {
    brush.Set();

    static AllocatedArray<GLushort> triangle_buffer;
    unsigned idx_count = PolygonToTriangles(points, num_points,
                                            triangle_buffer);
    if (idx_count > 0)
      glDrawElements(GL_TRIANGLES, idx_count, GL_UNSIGNED_SHORT,
                     triangle_buffer.begin());
  }

  if (pen_over_brush()) {
    pen.Bind();

    if (pen.GetWidth() <= 2) {
      glDrawArrays(GL_LINE_LOOP, 0, num_points);
    } else {
      unsigned vertices = LineToTriangles(points, num_points, vertex_buffer,
                                          pen.GetWidth(), true);
      if (vertices > 0) {
        glVertexPointer(2, GL_VALUE, 0, vertex_buffer.begin());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices);
      }
    }

    pen.Unbind();
  }
}

void
Canvas::DrawTriangleFan(const RasterPoint *points, unsigned num_points)
{
  if (brush.IsHollow() && !pen.IsDefined())
    return;

  glVertexPointer(2, GL_VALUE, 0, points);

  if (!brush.IsHollow() && num_points >= 3) {
    brush.Set();
    glDrawArrays(GL_TRIANGLE_FAN, 0, num_points);
  }

  if (pen_over_brush()) {
    pen.Bind();

    if (pen.GetWidth() <= 2) {
      glDrawArrays(GL_LINE_LOOP, 0, num_points);
    } else {
      unsigned vertices = LineToTriangles(points, num_points, vertex_buffer,
                                          pen.GetWidth(), true);
      if (vertices > 0) {
        glVertexPointer(2, GL_VALUE, 0, vertex_buffer.begin());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices);
      }
    }

    pen.Unbind();
  }
}

void
Canvas::DrawLine(PixelScalar ax, PixelScalar ay, PixelScalar bx, PixelScalar by)
{
  pen.Bind();

  const GLvalue v[] = { ax, ay, bx, by };
  glVertexPointer(2, GL_VALUE, 0, v);
  glDrawArrays(GL_LINE_STRIP, 0, 2);

  pen.Unbind();
}

/**
 * Draw a line from a to b, using triangle caps if pen-size > 2 to hide
 * gaps between consecutive lines.
 */
void
Canvas::DrawLinePiece(const RasterPoint a, const RasterPoint b)
{
  pen.Bind();

  const RasterPoint v[] = { {a.x, a.y}, {b.x, b.y} };
  if (pen.GetWidth() > 2) {
    unsigned strip_len = LineToTriangles(v, 2, vertex_buffer, pen.GetWidth(),
                                         false, true);
    if (strip_len > 0) {
      glVertexPointer(2, GL_VALUE, 0, vertex_buffer.begin());
      glDrawArrays(GL_TRIANGLE_STRIP, 0, strip_len);
    }
  } else {
    glVertexPointer(2, GL_VALUE, 0, &v[0].x);
    glDrawArrays(GL_LINE_STRIP, 0, 2);
  }

  pen.Unbind();
}

void
Canvas::DrawTwoLines(PixelScalar ax, PixelScalar ay,
                  PixelScalar bx, PixelScalar by,
                  PixelScalar cx, PixelScalar cy)
{
  pen.Bind();

  const GLvalue v[] = { ax, ay, bx, by, cx, cy };
  glVertexPointer(2, GL_VALUE, 0, v);
  glDrawArrays(GL_LINE_STRIP, 0, 3);

  pen.Unbind();
}

void
Canvas::DrawCircle(PixelScalar x, PixelScalar y, UPixelScalar radius)
{
  if (pen_over_brush() && pen.GetWidth() > 2) {
    GLDonutVertices vertices(x, y,
                             radius - pen.GetWidth() / 2,
                             radius + pen.GetWidth() / 2);
    if (!brush.IsHollow()) {
      vertices.bind_inner_circle();
      brush.Set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.CIRCLE_SIZE);
    }
    vertices.bind();
    pen.Set();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.SIZE);
  } else if (OpenGL::vertex_buffer_object && radius < 16) {
    /* draw a "small" circle with VBO */

    OpenGL::small_circle_buffer->Bind();
    glVertexPointer(2, GL_SHORT, 0, NULL);

    glPushMatrix();

#ifdef HAVE_GLES
    glTranslatex((GLfixed)x << 16, (GLfixed)y << 16, 0);
    glScalex((GLfixed)radius << 8, (GLfixed)radius << 8, (GLfixed)1 << 16);
#else
    glTranslatef(x, y, 0.);
    glScalef(radius / 256., radius / 256., 1.);
#endif

    if (!brush.IsHollow()) {
      brush.Set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, OpenGL::SMALL_CIRCLE_SIZE);
    }

    if (pen_over_brush()) {
      pen.Bind();
      glDrawArrays(GL_LINE_LOOP, 0, OpenGL::SMALL_CIRCLE_SIZE);
      pen.Unbind();
    }

    glPopMatrix();

    OpenGL::small_circle_buffer->Unbind();
  } else if (OpenGL::vertex_buffer_object) {
    /* draw a "big" circle with VBO */

    OpenGL::circle_buffer->Bind();
    glVertexPointer(2, GL_SHORT, 0, NULL);

    glPushMatrix();

#ifdef HAVE_GLES
    glTranslatex((GLfixed)x << 16, (GLfixed)y << 16, 0);
    glScalex((GLfixed)radius << 6, (GLfixed)radius << 6, (GLfixed)1 << 16);
#else
    glTranslatef(x, y, 0.);
    glScalef(radius / 1024., radius / 1024., 1.);
#endif

    if (!brush.IsHollow()) {
      brush.Set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, OpenGL::CIRCLE_SIZE);
    }

    if (pen_over_brush()) {
      pen.Bind();
      glDrawArrays(GL_LINE_LOOP, 0, OpenGL::CIRCLE_SIZE);
      pen.Unbind();
    }

    glPopMatrix();

    OpenGL::circle_buffer->Unbind();
  } else {
    GLCircleVertices vertices(x, y, radius);
    vertices.bind();

    if (!brush.IsHollow()) {
      brush.Set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.SIZE);
    }

    if (pen_over_brush()) {
      pen.Bind();
      glDrawArrays(GL_LINE_LOOP, 0, vertices.SIZE);
      pen.Unbind();
    }
  }
}

void
Canvas::DrawSegment(PixelScalar x, PixelScalar y, UPixelScalar radius,
                Angle start, Angle end, bool horizon)
{
  ::Segment(*this, x, y, radius, start, end, horizon);
}

gcc_const
static unsigned
AngleToDonutVertex(Angle angle)
{
  return GLDonutVertices::ImportAngle(NATIVE_TO_INT(angle.Native())
                                      + ARRAY_SIZE(ISINETABLE) * 3u / 4u,
                                      ARRAY_SIZE(ISINETABLE));
}

gcc_const
static std::pair<unsigned,unsigned>
AngleToDonutVertices(Angle start, Angle end)
{
  static const Angle epsilon = Angle::FullCircle()
    / (GLDonutVertices::CIRCLE_SIZE * 4u);

  const Angle delta = end - start;

  if (fabs(delta.AsDelta().Native()) <= epsilon.Native())
    /* full circle */
    return std::make_pair(0, GLDonutVertices::MAX_ANGLE);

  const unsigned istart = AngleToDonutVertex(start);
  unsigned iend = AngleToDonutVertex(end);

  if (istart == iend && delta > epsilon) {
    if (end - start >= Angle::HalfCircle())
      /* nearly full circle, round down the end */
      iend = GLDonutVertices::PreviousAngle(iend);
    else
      /* slightly larger than epsilon: draw at least two indices */
      iend = GLDonutVertices::NextAngle(iend);
  }

  return std::make_pair(istart, iend);
}

void
Canvas::DrawAnnulus(PixelScalar x, PixelScalar y,
                UPixelScalar small_radius, UPixelScalar big_radius,
                Angle start, Angle end)
{
  if (1 == 1) {
    /* TODO: switched to the unoptimised generic implementation due to
       TRAC #2221, caused by rounding error of start/end radial;
       should reimplement GLDonutVertices to use the exact start/end
       radial */
    ::Annulus(*this, x, y, big_radius, start, end, small_radius);
    return;
  }

  GLDonutVertices vertices(x, y, small_radius, big_radius);

  const std::pair<unsigned,unsigned> i = AngleToDonutVertices(start, end);
  const unsigned istart = i.first;
  const unsigned iend = i.second;

  if (!brush.IsHollow()) {
    brush.Set();
    vertices.bind();

    if (istart > iend) {
      glDrawArrays(GL_TRIANGLE_STRIP, istart,
                   GLDonutVertices::MAX_ANGLE - istart + 2);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, iend + 2);
    } else {
      glDrawArrays(GL_TRIANGLE_STRIP, istart, iend - istart + 2);
    }
  }

  if (pen_over_brush()) {
    pen.Bind();

    if (istart != iend && iend != GLDonutVertices::MAX_ANGLE) {
      if (brush.IsHollow())
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
      glDrawArrays(GL_LINE_STRIP, pstart,
                   GLDonutVertices::CIRCLE_SIZE - pstart + 1);
      glDrawArrays(GL_LINE_STRIP, 0, pend + 1);
    }

    vertices.bind_outer_circle();
    if (pstart < pend) {
      glDrawArrays(GL_LINE_STRIP, pstart, pend - pstart + 1);
    } else {
      glDrawArrays(GL_LINE_STRIP, pstart,
                   GLDonutVertices::CIRCLE_SIZE - pstart + 1);
      glDrawArrays(GL_LINE_STRIP, 0, pend + 1);
    }

    pen.Unbind();
  }
}

void
Canvas::DrawKeyhole(PixelScalar x, PixelScalar y,
                UPixelScalar small_radius, UPixelScalar big_radius,
                Angle start, Angle end)
{
  ::KeyHole(*this, x, y, big_radius, start, end, small_radius);
}

void
Canvas::DrawFocusRectangle(PixelRect rc)
{
  DrawOutlineRectangle(rc.left, rc.top, rc.right, rc.bottom, COLOR_DARK_GRAY);
}

void
Canvas::text(PixelScalar x, PixelScalar y, const TCHAR *text)
{
  assert(text != NULL);
  assert(ValidateUTF8(text));

#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  if (font == NULL)
    return;

  GLTexture *texture = TextCache::Get(font, text);
  if (texture == NULL)
    return;

  if (background_mode == OPAQUE)
    /* draw the opaque background */
    DrawFilledRectangle(x, y,
                        x + texture->GetWidth(), y + texture->GetHeight(),
                        background_color);

  GLEnable scope(GL_TEXTURE_2D);
  texture->Bind();
  GLLogicOp logic_op(GL_AND_INVERTED);

  if (background_mode != OPAQUE || background_color != COLOR_BLACK) {
    /* cut out the shape in black */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    texture->Draw(x, y);
  }

  if (text_color != COLOR_BLACK) {
    /* draw the text color on top */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    logic_op.set(GL_OR);
    text_color.Set();
    texture->Draw(x, y);
  }
}

void
Canvas::text_transparent(PixelScalar x, PixelScalar y, const TCHAR *text)
{
  assert(text != NULL);
  assert(ValidateUTF8(text));

#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  if (font == NULL)
    return;

  GLTexture *texture = TextCache::Get(font, text);
  if (texture == NULL)
    return;

  GLEnable scope(GL_TEXTURE_2D);
  texture->Bind();
  GLLogicOp logic_op(GL_AND_INVERTED);

  /* cut out the shape in black */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  texture->Draw(x, y);

  if (text_color != COLOR_BLACK) {
    /* draw the text color on top */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    logic_op.set(GL_OR);
    text_color.Set();
    texture->Draw(x, y);
  }
}

void
Canvas::TextClipped(PixelScalar x, PixelScalar y,
                    UPixelScalar width, UPixelScalar height,
                    const TCHAR *text)
{
  assert(text != NULL);
  assert(ValidateUTF8(text));

#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  if (font == NULL)
    return;

  GLTexture *texture = TextCache::Get(font, text);
  if (texture == NULL)
    return;

  GLEnable scope(GL_TEXTURE_2D);
  texture->Bind();
  GLLogicOp logic_op(GL_AND_INVERTED);

  if (texture->GetHeight() < height)
    height = texture->GetHeight();
  if (texture->GetWidth() < width)
    width = texture->GetWidth();

  /* cut out the shape in black */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  texture->Draw(x, y, width, height, 0, 0, width, height);

  if (text_color != COLOR_BLACK) {
    /* draw the text color on top */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    logic_op.set(GL_OR);
    text_color.Set();
    texture->Draw(x, y, width, height, 0, 0, width, height);
  }
}

void
Canvas::Stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const GLTexture &texture,
                PixelScalar src_x, PixelScalar src_y,
                UPixelScalar src_width, UPixelScalar src_height)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  texture.Draw(dest_x, dest_y, dest_width, dest_height,
               src_x, src_y, src_width, src_height);
}

void
Canvas::Stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const GLTexture &texture)
{
  Stretch(dest_x, dest_y, dest_width, dest_height,
          texture, 0, 0, texture.GetWidth(), texture.GetHeight());
}

void
Canvas::copy(PixelScalar dest_x, PixelScalar dest_y,
             UPixelScalar dest_width, UPixelScalar dest_height,
             const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  Stretch(dest_x, dest_y, dest_width, dest_height,
          src, src_x, src_y, dest_width, dest_height);
}

void
Canvas::copy(const Bitmap &src)
{
  copy(0, 0, src.GetWidth(), src.GetHeight(), src, 0, 0);
}

void
Canvas::stretch_transparent(const Bitmap &src, Color key)
{
  assert(src.IsDefined());

  // XXX
  Stretch(src);
}

void
Canvas::invert_stretch_transparent(const Bitmap &src, Color key)
{
  assert(src.IsDefined());

  // XXX
  GLLogicOp invert(GL_COPY_INVERTED);
  Stretch(src);
}

void
Canvas::Stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src, PixelScalar src_x, PixelScalar src_y,
                UPixelScalar src_width, UPixelScalar src_height)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif
  assert(src.IsDefined());

  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  GLTexture &texture = *src.GetNative();
  GLEnable scope(GL_TEXTURE_2D);
  texture.Bind();
  texture.Draw(dest_x, dest_y, dest_width, dest_height,
               src_x, src_y, src_width, src_height);
}

void
Canvas::Stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif
  assert(src.IsDefined());

  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  GLTexture &texture = *src.GetNative();
  GLEnable scope(GL_TEXTURE_2D);
  texture.Bind();
  texture.Draw(dest_x, dest_y, dest_width, dest_height,
               0, 0, src.GetWidth(), src.GetHeight());
}

void
Canvas::StretchAnd(PixelScalar dest_x, PixelScalar dest_y,
                   UPixelScalar dest_width, UPixelScalar dest_height,
                   const Bitmap &src,
                   PixelScalar src_x, PixelScalar src_y,
                   UPixelScalar src_width, UPixelScalar src_height)
{
  GLLogicOp logic_op(GL_AND);
  Stretch(dest_x, dest_y, dest_width, dest_height,
          src, src_x, src_y, src_width, src_height);
}

void
Canvas::StretchNotOr(PixelScalar dest_x, PixelScalar dest_y,
                     UPixelScalar dest_width, UPixelScalar dest_height,
                     const Bitmap &src,
                     PixelScalar src_x, PixelScalar src_y,
                     UPixelScalar src_width, UPixelScalar src_height)
{
  GLLogicOp logic_op(GL_OR_INVERTED);
  Stretch(dest_x, dest_y, dest_width, dest_height,
          src, src_x, src_y, src_width, src_height);
}

void
Canvas::StretchMono(PixelScalar dest_x, PixelScalar dest_y,
                    UPixelScalar dest_width, UPixelScalar dest_height,
                    const Bitmap &src,
                    PixelScalar src_x, PixelScalar src_y,
                    UPixelScalar src_width, UPixelScalar src_height,
                    Color fg_color, Color bg_color)
{
  /* note that this implementation ignores the background color; it is
     not mandatory, and we can assume that the background is already
     set; it is only being passed to this function because the GDI
     implementation will be faster when erasing the background
     again */

  GLTexture &texture = *src.GetNative();
  GLEnable scope(GL_TEXTURE_2D);
  texture.Bind();

  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  if (fg_color == COLOR_WHITE) {
    /* white text requested: use this trivial implementation */
    GLLogicOp logic_op(GL_OR_INVERTED);
    texture.Draw(dest_x, dest_y, dest_width, dest_height,
                 src_x, src_y, src_width, src_height);
    return;
  }

  /* apply the mask, pixels will be black then */
  GLLogicOp logic_op(GL_AND);
  if (bg_color != COLOR_BLACK)
    texture.Draw(dest_x, dest_y, dest_width, dest_height,
                 src_x, src_y, src_width, src_height);

  if (fg_color != COLOR_BLACK) {
    /* draw */

#ifndef HAVE_GLES
    if (fg_color != COLOR_WHITE) {
      /* XXX OpenGL/ES doesn't support GL_OPERAND0_RGB; we can't print
         colored mono images currently */

      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB,
                        GL_ONE_MINUS_SRC_COLOR);

      const GLfloat color[] = {
        GLfloat(fg_color.Red() / 256.),
        GLfloat(fg_color.Green() / 256.),
        GLfloat(fg_color.Blue() / 256.),
        GLfloat(1.0),
      };

      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
      glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

      logic_op.set(GL_OR);
    } else
#endif
      logic_op.set(GL_OR_INVERTED);

    texture.Draw(dest_x, dest_y, dest_width, dest_height,
                 src_x, src_y, src_width, src_height);
  }
}

void
Canvas::CopyOr(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.IsDefined());

  GLLogicOp logic_op(GL_OR);
  copy(dest_x, dest_y, dest_width, dest_height,
       src, src_x, src_y);
}

void
Canvas::CopyNotOr(PixelScalar dest_x, PixelScalar dest_y,
                  UPixelScalar dest_width, UPixelScalar dest_height,
                  const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.IsDefined());

  GLLogicOp logic_op(GL_OR_INVERTED);
  copy(dest_x, dest_y, dest_width, dest_height,
       src, src_x, src_y);
}

void
Canvas::CopyAnd(PixelScalar dest_x, PixelScalar dest_y,
                 UPixelScalar dest_width, UPixelScalar dest_height,
                 const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.IsDefined());

  GLLogicOp logic_op(GL_AND);
  copy(dest_x, dest_y, dest_width, dest_height,
       src, src_x, src_y);
}

void
Canvas::CopyNot(PixelScalar dest_x, PixelScalar dest_y,
                 UPixelScalar dest_width, UPixelScalar dest_height,
                 const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.IsDefined());

  GLLogicOp logic_op(GL_COPY_INVERTED);
  copy(dest_x, dest_y, dest_width, dest_height,
       src, src_x, src_y);
}

void
Canvas::CopyToTexture(GLTexture &texture, PixelRect src_rc) const
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  texture.Bind();
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                      OpenGL::translate_x + src_rc.left,
                      OpenGL::screen_height - OpenGL::translate_y - src_rc.bottom,
                      src_rc.right - src_rc.left,
                      src_rc.bottom - src_rc.top);

}

void
Canvas::DrawRoundRectangle(PixelScalar left, PixelScalar top,
                        PixelScalar right, PixelScalar bottom,
                        UPixelScalar ellipse_width,
                        UPixelScalar ellipse_height)
{
  UPixelScalar radius = std::min(std::min(ellipse_width, ellipse_height),
                                 (UPixelScalar) std::min(bottom - top,
                                                         right - left)) / 2;
  ::RoundRect(*this, left, top, right, bottom, radius);
}
