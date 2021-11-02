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

#include "Canvas.hpp"
#include "Triangulate.hpp"
#include "Globals.hpp"
#include "Texture.hpp"
#include "Scope.hpp"
#include "VertexArray.hpp"
#include "Shapes.hpp"
#include "Buffer.hpp"
#include "VertexPointer.hpp"
#include "ExactPixelPoint.hpp"
#include "ui/canvas/custom/Cache.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Util.hpp"
#include "ui/opengl/Features.hpp"
#include "Math/Angle.hpp"
#include "util/AllocatedArray.hxx"
#include "util/Macros.hpp"
#include "util/TStringView.hxx"
#include "util/UTF8.hpp"

#include "Shaders.hpp"
#include "Program.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef UNICODE
#include "util/ConvertString.hpp"
#endif

#ifndef NDEBUG
#include "util/UTF8.hpp"
#endif

#include <cassert>

AllocatedArray<BulkPixelPoint> Canvas::vertex_buffer;

void
Canvas::InvertRectangle(PixelRect r)
{
  /** Inverts rectangle using GL blending effects (hardware accelerated):
   *
   * Drawing white (Draw_color=1,1,1) rectangle over the image with GL_ONE_MINUS_DST_COLOR
   * blending function yields New_DST_color= Draw_Color*(1-Old_DST_Color)
   *
   */

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE); // Make sure alpha channel is not damaged

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO); // DST is overwritten part of image = old_DST_color

  const Color cwhite(0xff, 0xff, 0xff); // Draw color white (source channel of blender)

  DrawFilledRectangle(r, cwhite);

  glDisable(GL_BLEND);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

static TStringView
ClipText(const Font &font, TStringView text,
         int x, unsigned canvas_width) noexcept
{
  if (text.empty() || x >= int(canvas_width))
    return nullptr;

  /* this is an approximation, just good enough for clipping */
  unsigned font_width = std::max(font.GetHeight() / 4U, 1U);

  unsigned max_width = canvas_width - x;
  unsigned max_chars = max_width / font_width;

  text.size = TruncateStringUTF8(text, max_chars);
  return text;
}

void
Canvas::DrawFilledRectangle(PixelRect r, const Color color) noexcept
{
  assert(offset == OpenGL::translate);

  OpenGL::solid_shader->Use();

  color.Bind();

  /* can't use glRecti() with GLSL because it bypasses the vertex
     shader */

  const BulkPixelPoint vertices[] = {
    {r.left, r.top},
    {r.right, r.top},
    {r.left, r.bottom},
    {r.right, r.bottom},
  };

  const ScopeVertexPointer vp(vertices);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void
Canvas::DrawOutlineRectangleGL(PixelRect r) noexcept
{
  --r.right;
  --r.bottom;

  const ExactPixelPoint vertices[] = {
    r.GetTopLeft(),
    r.GetTopRight(),
    r.GetBottomRight(),
    r.GetBottomLeft(),
  };

  const ScopeVertexPointer vp(vertices);
  glDrawArrays(GL_LINE_LOOP, 0, 4);
}

void
Canvas::FadeToWhite(GLubyte alpha)
{
  const ScopeAlphaBlend alpha_blend;
  const Color color(0xff, 0xff, 0xff, alpha);
  Clear(color);
}

void
Canvas::FadeToWhite(PixelRect rc, GLubyte alpha)
{
  const ScopeAlphaBlend alpha_blend;
  const Color color(0xff, 0xff, 0xff, alpha);
  DrawFilledRectangle(rc, color);
}

void
Canvas::DrawPolyline(const BulkPixelPoint *points, unsigned num_points)
{
  OpenGL::solid_shader->Use();

  pen.Bind();

  const ScopeVertexPointer vp(points);
  glDrawArrays(GL_LINE_STRIP, 0, num_points);

  pen.Unbind();
}

void
Canvas::DrawPolygon(const BulkPixelPoint *points, unsigned num_points)
{
  if (brush.IsHollow() && !pen.IsDefined())
    return;

  OpenGL::solid_shader->Use();

  ScopeVertexPointer vp(points);

  if (!brush.IsHollow() && num_points >= 3) {
    brush.Bind();

    static AllocatedArray<GLushort> triangle_buffer;
    unsigned idx_count = PolygonToTriangles(points, num_points,
                                            triangle_buffer);
    if (idx_count > 0)
      glDrawElements(GL_TRIANGLES, idx_count, GL_UNSIGNED_SHORT,
                     triangle_buffer.begin());
  }

  if (IsPenOverBrush()) {
    pen.Bind();

    if (pen.GetWidth() <= 2) {
      glDrawArrays(GL_LINE_LOOP, 0, num_points);
    } else {
      unsigned vertices = LineToTriangles(points, num_points, vertex_buffer,
                                          pen.GetWidth(), true);
      if (vertices > 0) {
        vp.Update(vertex_buffer.begin());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices);
      }
    }

    pen.Unbind();
  }
}

void
Canvas::DrawTriangleFan(const BulkPixelPoint *points, unsigned num_points)
{
  if (brush.IsHollow() && !pen.IsDefined())
    return;

  OpenGL::solid_shader->Use();

  ScopeVertexPointer vp(points);

  if (!brush.IsHollow() && num_points >= 3) {
    brush.Bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, num_points);
  }

  if (IsPenOverBrush()) {
    pen.Bind();

    if (pen.GetWidth() <= 2) {
      glDrawArrays(GL_LINE_LOOP, 0, num_points);
    } else {
      unsigned vertices = LineToTriangles(points, num_points, vertex_buffer,
                                          pen.GetWidth(), true);
      if (vertices > 0) {
        vp.Update(vertex_buffer.begin());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices);
      }
    }

    pen.Unbind();
  }
}

void
Canvas::DrawHLine(int x1, int x2, int y, Color color)
{
  color.Bind();

  const BulkPixelPoint v[] = {
    { GLvalue(x1), GLvalue(y) },
    { GLvalue(x2), GLvalue(y) },
  };

  const ScopeVertexPointer vp(v);
  glDrawArrays(GL_LINE_STRIP, 0, ARRAY_SIZE(v));
}

void
Canvas::DrawLine(PixelPoint a, PixelPoint b) noexcept
{
  OpenGL::solid_shader->Use();

  pen.Bind();

  const BulkPixelPoint v[] = { a, b };
  const ScopeVertexPointer vp(v);
  glDrawArrays(GL_LINE_STRIP, 0, ARRAY_SIZE(v));

  pen.Unbind();
}

void
Canvas::DrawExactLine(PixelPoint a, PixelPoint b) noexcept
{
  OpenGL::solid_shader->Use();

  pen.Bind();

  const ExactPixelPoint v[] = { a, b };
  const ScopeVertexPointer vp(v);
  glDrawArrays(GL_LINE_STRIP, 0, ARRAY_SIZE(v));

  pen.Unbind();
}

/**
 * Draw a line from a to b, using triangle caps if pen-size > 2 to hide
 * gaps between consecutive lines.
 */
void
Canvas::DrawLinePiece(const PixelPoint a, const PixelPoint b)
{
  OpenGL::solid_shader->Use();

  pen.Bind();

  const BulkPixelPoint v[] = { {a.x, a.y}, {b.x, b.y} };
  if (pen.GetWidth() > 2) {
    unsigned strip_len = LineToTriangles(v, 2, vertex_buffer, pen.GetWidth(),
                                         false, true);
    if (strip_len > 0) {
      const ScopeVertexPointer vp(vertex_buffer.begin());
      glDrawArrays(GL_TRIANGLE_STRIP, 0, strip_len);
    }
  } else {
    const ScopeVertexPointer vp(v);
    glDrawArrays(GL_LINE_STRIP, 0, 2);
  }

  pen.Unbind();
}

void
Canvas::DrawTwoLines(PixelPoint a, PixelPoint b, PixelPoint c) noexcept
{
  OpenGL::solid_shader->Use();

  pen.Bind();

  const BulkPixelPoint v[] = { a, b, c };
  const ScopeVertexPointer vp(v);
  glDrawArrays(GL_LINE_STRIP, 0, ARRAY_SIZE(v));

  pen.Unbind();
}

void
Canvas::DrawTwoLinesExact(PixelPoint a, PixelPoint b, PixelPoint c) noexcept
{
  OpenGL::solid_shader->Use();

  pen.Bind();

  const ExactPixelPoint v[] = { a, b, c };
  const ScopeVertexPointer vp(v);
  glDrawArrays(GL_LINE_STRIP, 0, ARRAY_SIZE(v));

  pen.Unbind();
}

void
Canvas::DrawCircle(PixelPoint center, unsigned radius) noexcept
{
  OpenGL::solid_shader->Use();

  if (IsPenOverBrush() && pen.GetWidth() > 2) {
    ScopeVertexPointer vp;
    GLDonutVertices vertices(center.x, center.y,
                             radius - pen.GetWidth() / 2,
                             radius + pen.GetWidth() / 2);
    if (!brush.IsHollow()) {
      vertices.BindInnerCircle(vp);
      brush.Bind();
      glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.CIRCLE_SIZE);
    }
    vertices.Bind(vp);
    pen.Bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.SIZE);
    pen.Unbind();
  } else {
    auto &buffer = radius < 16
      ? *OpenGL::small_circle_buffer
      : *OpenGL::circle_buffer;
    const unsigned n = radius < 16
      ? OpenGL::SMALL_CIRCLE_SIZE
      : OpenGL::CIRCLE_SIZE;

    buffer.Bind();
    const auto points = (const FloatPoint2D *)nullptr;
    const ScopeVertexPointer vp(points);

    glm::mat4 matrix2 = glm::scale(glm::translate(glm::mat4(1),
                                                  glm::vec3(center.x, center.y, 0)),
                                   glm::vec3(GLfloat(radius), GLfloat(radius),
                                             1.));
    glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                       glm::value_ptr(matrix2));

    if (!brush.IsHollow()) {
      brush.Bind();
      glDrawArrays(GL_TRIANGLE_FAN, 0, n);
    }

    if (IsPenOverBrush()) {
      pen.Bind();
      glDrawArrays(GL_LINE_LOOP, 0, n);
      pen.Unbind();
    }

    glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(1)));

    buffer.Unbind();
  }
}

void
Canvas::DrawSegment(PixelPoint center, unsigned radius,
                    Angle start, Angle end, bool horizon)
{
  ::Segment(*this, center, radius, start, end, horizon);
}

void
Canvas::DrawArc(PixelPoint center, unsigned radius,
                Angle start, Angle end)
{
  ::Arc(*this, center, radius, start, end);
}

gcc_const
static unsigned
AngleToDonutVertex(Angle angle)
{
  return GLDonutVertices::ImportAngle(NATIVE_TO_INT(angle.Native())
                                      + ISINETABLE.size() * 3u / 4u,
                                      ISINETABLE.size());
}

gcc_const
static std::pair<unsigned,unsigned>
AngleToDonutVertices(Angle start, Angle end)
{
  static constexpr Angle epsilon = Angle::FullCircle()
    / int(GLDonutVertices::CIRCLE_SIZE * 4u);

  const Angle delta = end - start;

  if (fabs(delta.AsDelta().Native()) <= epsilon.Native())
    /* full circle */
    return std::make_pair(0u, unsigned(GLDonutVertices::MAX_ANGLE));

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
Canvas::DrawAnnulus(PixelPoint center,
                    unsigned small_radius, unsigned big_radius,
                    Angle start, Angle end)
{
  if (1 == 1) {
    /* TODO: switched to the unoptimised generic implementation due to
       TRAC #2221, caused by rounding error of start/end radial;
       should reimplement GLDonutVertices to use the exact start/end
       radial */
    ::Annulus(*this, center, big_radius, start, end, small_radius);
    return;
  }

  ScopeVertexPointer vp;
  GLDonutVertices vertices(center.x, center.y, small_radius, big_radius);

  const std::pair<unsigned,unsigned> i = AngleToDonutVertices(start, end);
  const unsigned istart = i.first;
  const unsigned iend = i.second;

  if (!brush.IsHollow()) {
    brush.Bind();
    vertices.Bind(vp);

    if (istart > iend) {
      glDrawArrays(GL_TRIANGLE_STRIP, istart,
                   GLDonutVertices::MAX_ANGLE - istart + 2);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, iend + 2);
    } else {
      glDrawArrays(GL_TRIANGLE_STRIP, istart, iend - istart + 2);
    }
  }

  if (IsPenOverBrush()) {
    pen.Bind();

    if (istart != iend && iend != GLDonutVertices::MAX_ANGLE) {
      if (brush.IsHollow())
        vertices.Bind(vp);

      glDrawArrays(GL_LINE_STRIP, istart, 2);
      glDrawArrays(GL_LINE_STRIP, iend, 2);
    }

    const unsigned pstart = istart / 2;
    const unsigned pend = iend / 2;

    vertices.BindInnerCircle(vp);
    if (pstart < pend) {
      glDrawArrays(GL_LINE_STRIP, pstart, pend - pstart + 1);
    } else {
      glDrawArrays(GL_LINE_STRIP, pstart,
                   GLDonutVertices::CIRCLE_SIZE - pstart + 1);
      glDrawArrays(GL_LINE_STRIP, 0, pend + 1);
    }

    vertices.BindOuterCircle(vp);
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
Canvas::DrawKeyhole(PixelPoint center,
                    unsigned small_radius, unsigned big_radius,
                    Angle start, Angle end)
{
  ::KeyHole(*this, center, big_radius, start, end, small_radius);
}

void
Canvas::DrawFocusRectangle(PixelRect rc)
{
  DrawOutlineRectangle(rc, COLOR_DARK_GRAY);
}

const PixelSize
Canvas::CalcTextSize(BasicStringView<TCHAR> text) const noexcept
{
  assert(text != nullptr);
#ifdef UNICODE
  const WideToUTF8Converter text2(text);
#else
  const StringView text2 = text;
  assert(ValidateUTF8(text));
#endif

  PixelSize size = { 0, 0 };

  if (font == nullptr)
    return size;

  /* see if the TextCache can handle this request */
  size = TextCache::LookupSize(*font, text2);
  if (size.height > 0)
    return size;

  return TextCache::GetSize(*font, text2);
}

/**
 * Prepare drawing a GL_ALPHA texture with the specified color.
 */
static void
PrepareColoredAlphaTexture(Color color)
{
  OpenGL::alpha_shader->Use();
  color.Bind();
}

void
Canvas::DrawText(PixelPoint p, BasicStringView<TCHAR> text) noexcept
{
  assert(text != nullptr);
#ifdef UNICODE
  const WideToUTF8Converter text2(text);
#else
  const StringView text2 = text;
  assert(ValidateUTF8(text));
#endif

#ifdef HAVE_GLES
  assert(offset == OpenGL::translate);
#endif

  if (font == nullptr)
    return;

  const StringView text3 = ClipText(*font, text2, p.x, size.width);
  if (text3.empty())
    return;

  GLTexture *texture = TextCache::Get(*font, text3);
  if (texture == nullptr)
    return;

  if (background_mode == OPAQUE)
    DrawFilledRectangle({p, texture->GetSize()}, background_color);

  PrepareColoredAlphaTexture(text_color);

  const ScopeAlphaBlend alpha_blend;

  texture->Bind();
  texture->Draw(p);
}

void
Canvas::DrawTransparentText(PixelPoint p, BasicStringView<TCHAR> text) noexcept
{
  assert(text != nullptr);
#ifdef UNICODE
  const WideToUTF8Converter text2(text);
#else
  const StringView text2 = text;
  assert(ValidateUTF8(text));
#endif

#ifdef HAVE_GLES
  assert(offset == OpenGL::translate);
#endif

  if (font == nullptr)
    return;

  const StringView text3 = ClipText(*font, text2, p.x, size.width);
  if (text3.empty())
    return;

  GLTexture *texture = TextCache::Get(*font, text3);
  if (texture == nullptr)
    return;

  PrepareColoredAlphaTexture(text_color);

  const ScopeAlphaBlend alpha_blend;

  texture->Bind();
  texture->Draw(p);
}

void
Canvas::DrawClippedText(PixelPoint p, PixelSize size,
                        BasicStringView<TCHAR> text) noexcept
{
  assert(text != nullptr);
#ifdef UNICODE
  const WideToUTF8Converter text2(text);
#else
  const StringView text2 = text;
  assert(ValidateUTF8(text));
#endif

#ifdef HAVE_GLES
  assert(offset == OpenGL::translate);
#endif

  if (font == nullptr)
    return;

  const StringView text3 = ClipText(*font, text2, 0, size.width);
  if (text3.empty())
    return;

  GLTexture *texture = TextCache::Get(*font, text3);
  if (texture == nullptr)
    return;

  if (texture->GetHeight() < size.height)
    size.height = texture->GetHeight();
  if (texture->GetWidth() < size.width)
    size.width = texture->GetWidth();

  PrepareColoredAlphaTexture(text_color);

  const ScopeAlphaBlend alpha_blend;

  texture->Bind();
  texture->Draw({p, size}, PixelRect{size});
}

void
Canvas::Stretch(PixelPoint dest_position, PixelSize dest_size,
                const GLTexture &texture,
                PixelPoint src_position, PixelSize src_size) noexcept
{
#ifdef HAVE_GLES
  assert(offset == OpenGL::translate);
#endif

  OpenGL::texture_shader->Use();

  texture.Draw({dest_position, dest_size}, {src_position, src_size});
}

void
Canvas::Stretch(PixelPoint dest_position, PixelSize dest_size,
                const GLTexture &texture)
{
  Stretch(dest_position, dest_size,
          texture, {0, 0}, texture.GetSize());
}

void
Canvas::Copy(PixelPoint dest_position, PixelSize dest_size,
             const Bitmap &src, PixelPoint src_position) noexcept
{
  Stretch(dest_position, dest_size,
          src, src_position, dest_size);
}

void
Canvas::Copy(const Bitmap &src)
{
  Copy({0, 0}, src.GetSize(), src, {0, 0});
}

void
Canvas::StretchNot(const Bitmap &src)
{
  assert(src.IsDefined());

  OpenGL::invert_shader->Use();

  GLTexture &texture = *src.GetNative();
  texture.Bind();
  texture.Draw(GetRect(), PixelRect(src.GetSize()));
}

void
Canvas::Stretch(PixelPoint dest_position, PixelSize dest_size,
                const Bitmap &src,
                PixelPoint src_position, PixelSize src_size) noexcept
{
#ifdef HAVE_GLES
  assert(offset == OpenGL::translate);
#endif
  assert(src.IsDefined());

  OpenGL::texture_shader->Use();

  GLTexture &texture = *src.GetNative();
  texture.Bind();
  texture.Draw({dest_position, dest_size}, {src_position, src_size});
}

void
Canvas::Stretch(PixelPoint dest_position, PixelSize dest_size,
                const Bitmap &src)
{
#ifdef HAVE_GLES
  assert(offset == OpenGL::translate);
#endif
  assert(src.IsDefined());

  OpenGL::texture_shader->Use();

  GLTexture &texture = *src.GetNative();
  texture.Bind();

  texture.Draw({dest_position, dest_size}, PixelRect{src.GetSize()});
}

void
Canvas::StretchMono(PixelPoint dest_position, PixelSize dest_size,
                    const Bitmap &src,
                    PixelPoint src_position, PixelSize src_size,
                    Color fg_color, Color bg_color)
{
  /* note that this implementation ignores the background color; it is
     not mandatory, and we can assume that the background is already
     set; it is only being passed to this function because the GDI
     implementation will be faster when erasing the background
     again */

  PrepareColoredAlphaTexture(fg_color);

  const ScopeAlphaBlend alpha_blend;

  GLTexture &texture = *src.GetNative();
  texture.Bind();
  texture.Draw({dest_position, dest_size}, {src_position, src_size});
}

void
Canvas::CopyToTexture(GLTexture &texture, PixelRect src_rc) const
{
#ifdef HAVE_GLES
  assert(offset == OpenGL::translate);
#endif

  texture.Bind();
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                      OpenGL::translate.x + src_rc.left,
                      OpenGL::viewport_size.y - OpenGL::translate.y - src_rc.bottom,
                      src_rc.GetWidth(), src_rc.GetHeight());

}

void
Canvas::DrawRoundRectangle(PixelRect r, PixelSize ellipse_size) noexcept
{
  unsigned radius = std::min(std::min(ellipse_size.width, ellipse_size.height),
                             std::min(r.GetWidth(),
                                      r.GetHeight())) / 2u;
  ::RoundRect(*this, r, radius);
}
