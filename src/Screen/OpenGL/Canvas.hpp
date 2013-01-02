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

#ifndef XCSOAR_SCREEN_OPENGL_CANVAS_HPP
#define XCSOAR_SCREEN_OPENGL_CANVAS_HPP

#include "Util/NonCopyable.hpp"
#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Font.hpp"
#include "Screen/Pen.hpp"
#include "Screen/OpenGL/Color.hpp"
#include "Screen/OpenGL/Point.hpp"
#include "Screen/OpenGL/Triangulate.hpp"
#include "Screen/OpenGL/Features.hpp"
#include "Util/AllocatedArray.hpp"
#include "Compiler.h"

#include <assert.h>
#include <tchar.h>

#ifdef HAVE_GLES
#include <GLES/gl.h>
#else
#include <SDL/SDL_opengl.h>
#endif

class Bitmap;
class GLTexture;

/**
 * Base drawable canvas class
 * 
 */
class Canvas : private NonCopyable {
  friend class SubCanvas;
  friend class BufferCanvas;

protected:
  PixelScalar x_offset, y_offset;
  UPixelScalar width, height;

  Pen pen;
  Brush brush;
  const Font *font;
  Color text_color, background_color;
  enum {
    OPAQUE, TRANSPARENT
  } background_mode;

  /**
   * static buffer to store vertices of wide lines.
   */
  static AllocatedArray<RasterPoint> vertex_buffer;

public:
  Canvas()
    :x_offset(0), y_offset(0), width(0), height(0),
     font(NULL), background_mode(OPAQUE) {}
  Canvas(UPixelScalar _width, UPixelScalar _height)
    :width(_width), height(_height),
     font(NULL), background_mode(OPAQUE) {}

  void set(UPixelScalar _width, UPixelScalar _height) {
    width = _width;
    height = _height;
  }

protected:
  /**
   * Returns true if the outline should be drawn after the area has
   * been filled.  As an optimization, this function returns false if
   * brush and pen share the same color.
   */
  bool pen_over_brush() const {
    return pen.IsDefined() &&
      (brush.IsHollow() || brush.GetColor() != pen.GetColor());
  }

public:
  bool IsDefined() const {
    return true;
  }

  UPixelScalar get_width() const {
    return width;
  }

  UPixelScalar get_height() const {
    return height;
  }

  void SelectNullPen() {
    pen = Pen(0, COLOR_BLACK);
  }

  void SelectWhitePen() {
    pen = Pen(1, COLOR_WHITE);
  }

  void SelectBlackPen() {
    pen = Pen(1, COLOR_BLACK);
  }

  void SelectHollowBrush() {
    brush.Reset();
  }

  void SelectWhiteBrush() {
    brush = Brush(COLOR_WHITE);
  }

  void SelectBlackBrush() {
    brush = Brush(COLOR_BLACK);
  }

  void Select(const Pen &_pen) {
    pen = _pen;
  }

  void Select(const Brush &_brush) {
    brush = _brush;
  }

  void Select(const Font &_font) {
    font = &_font;
  }

  void SetTextColor(const Color c) {
    text_color = c;
  }

  Color GetTextColor() const {
    return text_color;
  }

  void SetBackgroundColor(const Color c) {
    background_color = c;
  }

  Color GetBackgroundColor() const {
    return background_color;
  }

  void SetBackgroundOpaque() {
    background_mode = OPAQUE;
  }

  void SetBackgroundTransparent() {
    background_mode = TRANSPARENT;
  }

  void Rectangle(PixelScalar left, PixelScalar top,
                 PixelScalar right, PixelScalar bottom) {
    DrawFilledRectangle(left, top, right, bottom, brush);

    if (pen_over_brush())
      DrawOutlineRectangle(left, top, right, bottom);
  }

  void DrawFilledRectangle(PixelScalar left, PixelScalar top,
                      PixelScalar right, PixelScalar bottom,
                      const Color color);

  void DrawFilledRectangle(PixelScalar left, PixelScalar top,
                      PixelScalar right, PixelScalar bottom,
                      const Brush &brush) {
    if (!brush.IsHollow())
      DrawFilledRectangle(left, top, right, bottom, brush.GetColor());
  }

  void DrawFilledRectangle(const PixelRect &rc, const Color color) {
    DrawFilledRectangle(rc.left, rc.top, rc.right, rc.bottom, color);
  }

  void DrawFilledRectangle(const PixelRect rc, const Brush &brush) {
    DrawFilledRectangle(rc.left, rc.top, rc.right, rc.bottom, brush);
  }

  /**
   * Draw a rectangle outline with the current OpenGL color and
   * settings.
   */
  void OutlineRectangleGL(PixelScalar left, PixelScalar top,
                          PixelScalar right, PixelScalar bottom);

  void DrawOutlineRectangle(PixelScalar left, PixelScalar top,
                         PixelScalar right, PixelScalar bottom) {
    pen.Set();
    OutlineRectangleGL(left, top, right, bottom);
  }

  void DrawOutlineRectangle(PixelScalar left, PixelScalar top,
                         PixelScalar right, PixelScalar bottom,
                         Color color) {
    color.Set();
#ifdef HAVE_GLES
    glLineWidthx(1 << 16);
#else
    glLineWidth(1);
#endif

    OutlineRectangleGL(left, top, right, bottom);
  }

  /**
   * Fade to white.  This enables GL_BLEND and disables it before
   * returning.
   *
   * @param alpha the alpha value, 0=no change, 0xff=fully white.
   */
  void FadeToWhite(PixelRect rc, GLubyte alpha);
  void FadeToWhite(GLubyte alpha);

  void Clear() {
    Rectangle(0, 0, get_width(), get_height());
  }

  void Clear(const Color color) {
    DrawFilledRectangle(0, 0, get_width(), get_height(), color);
  }

  void Clear(const Brush &brush) {
    DrawFilledRectangle(0, 0, get_width(), get_height(), brush);
  }

  void ClearWhite() {
    Clear(COLOR_WHITE);
  }

  void DrawRoundRectangle(PixelScalar left, PixelScalar top,
                       PixelScalar right, PixelScalar bottom,
                       UPixelScalar ellipse_width,
                       UPixelScalar ellipse_height);

  void DrawRaisedEdge(PixelRect &rc);

  void DrawPolyline(const RasterPoint *points, unsigned num_points);

  void DrawPolygon(const RasterPoint *points, unsigned num_points);

  /**
   * Draw a triangle fan (GL_TRIANGLE_FAN).  The first point is the
   * origin of the fan.
   */
  void DrawTriangleFan(const RasterPoint *points, unsigned num_points);

  void DrawLine(PixelScalar ax, PixelScalar ay, PixelScalar bx, PixelScalar by);

  void DrawLine(const RasterPoint a, const RasterPoint b) {
    DrawLine(a.x, a.y, b.x, b.y);
  }

  void DrawLinePiece(const RasterPoint a, const RasterPoint b);

  void DrawTwoLines(PixelScalar ax, PixelScalar ay,
                 PixelScalar bx, PixelScalar by,
                 PixelScalar cx, PixelScalar cy);
  void DrawTwoLines(const RasterPoint a, const RasterPoint b,
                 const RasterPoint c) {
    DrawTwoLines(a.x, a.y, b.x, b.y, c.x, c.y);
  }

  void DrawCircle(PixelScalar x, PixelScalar y, UPixelScalar radius);

  void DrawSegment(PixelScalar x, PixelScalar y, UPixelScalar radius,
               Angle start, Angle end, bool horizon=false);

  void DrawAnnulus(PixelScalar x, PixelScalar y, UPixelScalar small_radius,
               UPixelScalar big_radius,
               Angle start, Angle end);

  void DrawKeyhole(PixelScalar x, PixelScalar y, UPixelScalar small_radius,
               UPixelScalar big_radius,
               Angle start, Angle end);

  void DrawFocusRectangle(PixelRect rc);

  void DrawButton(PixelRect rc, bool down);

  gcc_pure
  const PixelSize CalcTextSize(const TCHAR *text, size_t length) const;

  gcc_pure
  const PixelSize CalcTextSize(const TCHAR *text) const;

  gcc_pure
  UPixelScalar CalcTextWidth(const TCHAR *text) const {
    return CalcTextSize(text).cx;
  }

  gcc_pure
  UPixelScalar GetFontHeight() const {
    return font != NULL ? font->GetHeight() : 0;
  }

  void text(PixelScalar x, PixelScalar y, const TCHAR *text);
  void text(PixelScalar x, PixelScalar y, const TCHAR *text, size_t length);

  void text_transparent(PixelScalar x, PixelScalar y, const TCHAR *text);

  void text_opaque(PixelScalar x, PixelScalar y, const PixelRect &rc,
                   const TCHAR *text);

  void text_clipped(PixelScalar x, PixelScalar y, const PixelRect &rc,
                    const TCHAR *text) {
    // XXX

    if (x < rc.right)
      text_clipped(x, y, rc.right - x, text);
  }

  void TextClipped(PixelScalar x, PixelScalar y,
                   UPixelScalar width, UPixelScalar height,
                   const TCHAR *text);

  void text_clipped(PixelScalar x, PixelScalar y, UPixelScalar width,
                    const TCHAR *text) {
    TextClipped(x, y, width, 16384, text);
  }

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(PixelScalar x, PixelScalar y, const TCHAR *t) {
    if (x < (int)get_width() && y < (int)get_height())
      TextClipped(x, y, get_width() - x, get_height() - y, t);
  }

  void formatted_text(PixelRect *rc, const TCHAR *text, unsigned format);

  /**
   * Draws a texture.  The caller is responsible for binding it and
   * enabling GL_TEXTURE_2D.
   */
  void Stretch(PixelScalar dest_x, PixelScalar dest_y,
               UPixelScalar dest_width, UPixelScalar dest_height,
               const GLTexture &texture,
               PixelScalar src_x, PixelScalar src_y,
               UPixelScalar src_width, UPixelScalar src_height);

  void Stretch(PixelScalar dest_x, PixelScalar dest_y,
               UPixelScalar dest_width, UPixelScalar dest_height,
               const GLTexture &texture);


  void copy(PixelScalar dest_x, PixelScalar dest_y,
            UPixelScalar dest_width, UPixelScalar dest_height,
            const Bitmap &src, PixelScalar src_x, PixelScalar src_y);
  void copy(const Bitmap &src);

  void stretch_transparent(const Bitmap &src, Color key);
  void invert_stretch_transparent(const Bitmap &src, Color key);

  void Stretch(PixelScalar dest_x, PixelScalar dest_y,
               UPixelScalar dest_width, UPixelScalar dest_height,
               const Bitmap &src,
               PixelScalar src_x, PixelScalar src_y,
               UPixelScalar src_width, UPixelScalar src_height);
  void Stretch(PixelScalar dest_x, PixelScalar dest_y,
               UPixelScalar dest_width, UPixelScalar dest_height,
               const Bitmap &src);

  void Stretch(const Bitmap &src) {
    Stretch(0, 0, width, height, src);
  }

  void StretchAnd(PixelScalar dest_x, PixelScalar dest_y,
                  UPixelScalar dest_width, UPixelScalar dest_height,
                  const Bitmap &src,
                  PixelScalar src_x, PixelScalar src_y,
                  UPixelScalar src_width, UPixelScalar src_height);

  void StretchNotOr(PixelScalar dest_x, PixelScalar dest_y,
                    UPixelScalar dest_width, UPixelScalar dest_height,
                    const Bitmap &src,
                    PixelScalar src_x, PixelScalar src_y,
                    UPixelScalar src_width, UPixelScalar src_height);

  /**
   * Stretches a monochrome bitmap (1 bit per pixel), painting the
   * black pixels in the specified foreground color.  The white pixels
   * will be either transparent or drawn in the specified background
   * color, whichever operation is faster on the Canvas.
   *
   * @param fg_color draw this color instead of "black"
   * @param bg_color draw this color instead of "white"
   */
  void StretchMono(PixelScalar dest_x, PixelScalar dest_y,
                   UPixelScalar dest_width, UPixelScalar dest_height,
                   const Bitmap &src,
                   PixelScalar src_x, PixelScalar src_y,
                   UPixelScalar src_width, UPixelScalar src_height,
                   Color fg_color, Color bg_color);

  void CopyOr(PixelScalar dest_x, PixelScalar dest_y,
               UPixelScalar dest_width, UPixelScalar dest_height,
               const Bitmap &src, PixelScalar src_x, PixelScalar src_y);

  void CopyOr(const Bitmap &src) {
    CopyOr(0, 0, get_width(), get_height(), src, 0, 0);
  }

  void CopyNotOr(PixelScalar dest_x, PixelScalar dest_y,
                 UPixelScalar dest_width, UPixelScalar dest_height,
                 const Bitmap &src, PixelScalar src_x, PixelScalar src_y);

  void CopyNot(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src, PixelScalar src_x, PixelScalar src_y);

  void CopyAnd(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src, PixelScalar src_x, PixelScalar src_y);

  void CopyAnd(const Bitmap &src) {
    CopyAnd(0, 0, get_width(), get_height(), src, 0, 0);
  }

  void ScaleCopy(PixelScalar dest_x, PixelScalar dest_y,
                  const Bitmap &src,
                  PixelScalar src_x, PixelScalar src_y,
                  UPixelScalar src_width, UPixelScalar src_height);

  /**
   * Copy pixels from this object to a texture.  The texture must be
   * initialised already.  Note that the texture will be flipped
   * vertically, and to draw it back to the screen, you need
   * GLTexture::DrawFlipped().
   */
  void CopyToTexture(GLTexture &texture, PixelRect src_rc) const;
};

#endif
