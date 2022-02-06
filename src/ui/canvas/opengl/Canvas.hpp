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

#pragma once

#include "Color.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "ui/opengl/System.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"
#include "util/StringView.hxx"

#include <tchar.h>


/* Workaround: Some Win32 headers define OPAQUE and TRANSPARENT as preprocessor
 * defines. Undefine them to avoid name conflict. */
#ifdef OPAQUE
#undef OPAQUE
#endif

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif


class Angle;
class Bitmap;
class GLTexture;
template<class T> class AllocatedArray;

/**
 * Base drawable canvas class
 * 
 */
class Canvas {
  friend class SubCanvas;
  friend class BufferCanvas;

protected:
  PixelPoint offset = {0, 0};
  PixelSize size = {0, 0};

  Pen pen;
  Brush brush;
  const Font *font = nullptr;
  Color text_color, background_color;
  enum {
    OPAQUE, TRANSPARENT
  } background_mode = OPAQUE;

  /**
   * static buffer to store vertices of wide lines.
   */
  static AllocatedArray<BulkPixelPoint> vertex_buffer;

public:
  Canvas() = default;
  Canvas(PixelSize _size):size(_size) {}

  Canvas(const Canvas &other) = delete;
  Canvas &operator=(const Canvas &other) = delete;

  void Create(PixelSize _size) {
    size = _size;
  }

protected:
  /**
   * Returns true if the outline should be drawn after the area has
   * been filled.  As an optimization, this function returns false if
   * brush and pen share the same color.
   */
  bool IsPenOverBrush() const {
    return pen.IsDefined() &&
      (brush.IsHollow() || brush.GetColor() != pen.GetColor());
  }

public:
  bool IsDefined() const {
    return true;
  }

  PixelSize GetSize() const {
    return size;
  }

  unsigned GetWidth() const {
    return size.width;
  }

  unsigned GetHeight() const {
    return size.height;
  }

  [[gnu::pure]]
  PixelRect GetRect() const {
    return PixelRect(size);
  }

  void SelectNullPen() {
    pen = Pen(0, COLOR_BLACK);
  }

  void SelectWhitePen() {
    pen = Pen(1, COLOR_WHITE);
  }

  void SelectWhitePen(unsigned width) {
    pen = Pen(width, COLOR_WHITE);
  }

  void SelectBlackPen() {
    pen = Pen(1, COLOR_BLACK);
  }

  void SelectBlackPen(unsigned width) {
    pen = Pen(width, COLOR_BLACK);
  }

  void SelectHollowBrush() {
    brush.Destroy();
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

  void InvertRectangle(PixelRect r);

  void DrawRectangle(PixelRect r) noexcept {
    DrawFilledRectangle(r, brush);

    if (IsPenOverBrush())
      DrawOutlineRectangle(r);
  }

  void DrawFilledRectangle(PixelRect r, const Color color) noexcept;

  void DrawFilledRectangle(PixelRect r, const Brush &brush) noexcept {
    if (!brush.IsHollow())
      DrawFilledRectangle(r, brush.GetColor());
  }

  /**
   * Draw a rectangle outline with the current OpenGL color and
   * settings.
   */
  void DrawOutlineRectangleGL(PixelRect r) noexcept;

  void DrawOutlineRectangle(PixelRect r) noexcept;
  void DrawOutlineRectangle(PixelRect r, Color color) noexcept;

  /**
   * Fade to white.  This enables GL_BLEND and disables it before
   * returning.
   *
   * @param alpha the alpha value, 0=no change, 0xff=fully white.
   */
  void FadeToWhite(PixelRect rc, GLubyte alpha);
  void FadeToWhite(GLubyte alpha);

  void Clear() {
    DrawRectangle(PixelRect{GetSize()});
  }

  void Clear(const Color color) {
    DrawFilledRectangle(PixelRect{GetSize()}, color);
  }

  void Clear(const Brush &brush) {
    DrawFilledRectangle(PixelRect{GetSize()}, brush);
  }

  void ClearWhite() {
    Clear(COLOR_WHITE);
  }

  void DrawRoundRectangle(PixelRect r, PixelSize ellipse_size) noexcept;

  void DrawRaisedEdge(PixelRect &rc) noexcept;

  void DrawPolyline(const BulkPixelPoint *points, unsigned num_points);

  void DrawPolygon(const BulkPixelPoint *points, unsigned num_points);

  /**
   * Draw a triangle fan (GL_TRIANGLE_FAN).  The first point is the
   * origin of the fan.
   */
  void DrawTriangleFan(const BulkPixelPoint *points, unsigned num_points);

  /**
   * Draw a solid thin horizontal line.
   */
  void DrawHLine(int x1, int x2, int y, Color color);

  void DrawLine(PixelPoint a, PixelPoint b) noexcept;

  /**
   * Similar to DrawLine(), but force exact pixel coordinates.  This
   * may be more expensive on some platforms, and works only for thin
   * lines.
   */
  void DrawExactLine(PixelPoint a, PixelPoint b) noexcept;

  void DrawLinePiece(const PixelPoint a, const PixelPoint b);

  void DrawTwoLines(PixelPoint a, PixelPoint b, PixelPoint c) noexcept;

  /**
   * @see DrawTwoLines(), DrawExactLine()
   */
  void DrawTwoLinesExact(PixelPoint a, PixelPoint b, PixelPoint c) noexcept;

  void DrawCircle(PixelPoint center, unsigned radius) noexcept;

  void DrawSegment(PixelPoint center, unsigned radius,
                   Angle start, Angle end, bool horizon=false);

  void DrawAnnulus(PixelPoint center, unsigned small_radius,
                   unsigned big_radius,
                   Angle start, Angle end);

  void DrawKeyhole(PixelPoint center, unsigned small_radius,
                   unsigned big_radius,
                   Angle start, Angle end);

  void DrawArc(PixelPoint center, unsigned radius,
               Angle start, Angle end);

  void DrawFocusRectangle(PixelRect rc);

  [[gnu::pure]]
  const PixelSize CalcTextSize(BasicStringView<TCHAR> text) const noexcept;

  [[gnu::pure]]
  unsigned CalcTextWidth(BasicStringView<TCHAR> text) const noexcept {
    return CalcTextSize(text).width;
  }

  [[gnu::pure]]
  unsigned GetFontHeight() const {
    return font != nullptr ? font->GetHeight() : 0;
  }

  void DrawText(PixelPoint p, BasicStringView<TCHAR> text) noexcept;

  void DrawTransparentText(PixelPoint p, BasicStringView<TCHAR> text) noexcept;

  void DrawOpaqueText(PixelPoint p, const PixelRect &rc,
                      BasicStringView<TCHAR> text) noexcept;

  void DrawClippedText(PixelPoint p, const PixelRect &rc,
                       BasicStringView<TCHAR> text) noexcept {
    // XXX

    if (p.x < rc.right)
      DrawClippedText(p, rc.right - p.x, text);
  }

  void DrawClippedText(PixelPoint p, PixelSize size,
                       BasicStringView<TCHAR> text) noexcept;

  void DrawClippedText(PixelPoint p, unsigned width,
                       BasicStringView<TCHAR> text) noexcept {
    DrawClippedText(p, {width, 16384u}, text);
  }

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(PixelPoint p, BasicStringView<TCHAR> t) noexcept {
    if (p.x < (int)GetWidth() && p.y < (int)GetHeight())
      DrawClippedText(p, {GetWidth() - p.x, GetHeight() - p.y}, t);
  }

  /**
   * Render multi-line text.
   *
   * @return the resulting text height
   */
  unsigned DrawFormattedText(PixelRect r, BasicStringView<TCHAR> text,
                             unsigned format) noexcept;

  /**
   * Draws a texture.  The caller is responsible for binding it and
   * enabling GL_TEXTURE_2D.
   */
  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const GLTexture &texture,
               PixelPoint src_position, PixelSize src_size) noexcept;

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const GLTexture &texture);

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            const Bitmap &src, PixelPoint src_position) noexcept;
  void Copy(const Bitmap &src);

  void StretchNot(const Bitmap &src);

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src,
               PixelPoint src_position, PixelSize src_size) noexcept;
  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src);

  void Stretch(const Bitmap &src) {
    Stretch({0,0}, size, src);
  }

  /**
   * Stretches a monochrome bitmap (1 bit per pixel), painting the
   * black pixels in the specified foreground color.  The white pixels
   * will be either transparent or drawn in the specified background
   * color, whichever operation is faster on the Canvas.
   *
   * @param fg_color draw this color instead of "black"
   * @param bg_color draw this color instead of "white"
   */
  void StretchMono(PixelPoint dest_position, PixelSize dest_size,
                   const Bitmap &src,
                   PixelPoint src_position, PixelSize src_size,
                   Color fg_color, Color bg_color);

  void ScaleCopy(PixelPoint dest_position,
                 const Bitmap &src,
                 PixelPoint src_position, PixelSize src_size) noexcept;

  /**
   * Copy pixels from this object to a texture.  The texture must be
   * initialised already.  Note that the texture will be flipped
   * vertically. So the texture must be created with flipped=true.
   */
  void CopyToTexture(GLTexture &texture, PixelRect src_rc) const;
};
