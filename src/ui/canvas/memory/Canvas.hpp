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

#ifndef XCSOAR_SCREEN_MEMORY_CANVAS_HPP
#define XCSOAR_SCREEN_MEMORY_CANVAS_HPP

#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"
#include "Color.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "PixelTraits.hpp"
#include "Buffer.hpp"
#include "ActivePixelTraits.hpp"
#include "util/Compiler.h"

#include <tchar.h>

#ifdef _WIN32
/* those are WIN32 macros - undefine, or Canvas::background_mode will
   break */
#undef OPAQUE
#undef TRANSPARENT
#endif

class Angle;
class Bitmap;

/**
 * Base drawable canvas class
 */
class Canvas {
  friend class WindowCanvas;
  friend class SubCanvas;

  using ConstImageBuffer = ::ConstImageBuffer<ActivePixelTraits>;

protected:
  WritableImageBuffer<ActivePixelTraits> buffer;

  Pen pen;
  Brush brush;
  const Font *font = nullptr;
  Color text_color, background_color;
  enum {
    OPAQUE, TRANSPARENT
  } background_mode = OPAQUE;

public:
  Canvas()
    :buffer(WritableImageBuffer<ActivePixelTraits>::Empty()) {}

  explicit Canvas(WritableImageBuffer<ActivePixelTraits> _buffer)
    :buffer(_buffer) {}

  void Create(WritableImageBuffer<ActivePixelTraits> _buffer) {
    buffer = _buffer;
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
    return buffer.data != nullptr;
  }

  PixelSize GetSize() const {
    return { buffer.width, buffer.height };
  }

  unsigned GetWidth() const {
    return buffer.width;
  }

  unsigned GetHeight() const {
    return buffer.height;
  }

  gcc_pure
  PixelRect GetRect() const {
    return PixelRect(GetSize());
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

  void DrawOutlineRectangle(int left, int top, int right, int bottom,
                            Color color);

  void Rectangle(int left, int top, int right, int bottom) {
    DrawFilledRectangle(left, top, right, bottom, brush);

    if (IsPenOverBrush())
      DrawOutlineRectangle(left, top, right, bottom, pen.GetColor());
  }

  void DrawFilledRectangle(int left, int top, int right, int bottom,
                           Color color);

  void DrawFilledRectangle(int left, int top, int right, int bottom,
                           const Brush &brush) {
    if (brush.IsHollow())
      return;

    DrawFilledRectangle(left, top, right, bottom, brush.GetColor());
  }

  void DrawFilledRectangle(const PixelRect &rc, const Color color) {
    DrawFilledRectangle(rc.left, rc.top, rc.right, rc.bottom, color);
  }

  void DrawFilledRectangle(const PixelRect &rc, const Brush &brush) {
    DrawFilledRectangle(rc.left, rc.top, rc.right, rc.bottom, brush);
  }

  void InvertRectangle(PixelRect r);

  void Clear() {
    Rectangle(0, 0, GetWidth(), GetHeight());
  }

  void Clear(const Color color) {
    DrawFilledRectangle(0, 0, GetWidth(), GetHeight(), color);
  }

  void Clear(const Brush &brush) {
    DrawFilledRectangle(0, 0, GetWidth(), GetHeight(), brush);
  }

  void ClearWhite() {
    Clear(COLOR_WHITE);
  }

  void DrawRoundRectangle(int left, int top, int right, int bottom,
                          unsigned ellipse_width, unsigned ellipse_height);

  void DrawRaisedEdge(PixelRect &rc) {
    Pen bright(1, Color(240, 240, 240));
    Select(bright);
    DrawTwoLines(rc.left, rc.bottom - 2, rc.left, rc.top,
                 rc.right - 1, rc.top);

    Pen dark(1, Color(128, 128, 128));
    Select(dark);
    DrawTwoLines(rc.left, rc.bottom - 1, rc.right - 1, rc.bottom - 1,
              rc.right - 1, rc.top + 1);

    ++rc.left;
    ++rc.top;
    --rc.right;
    --rc.bottom;
  }

  void DrawPolyline(const BulkPixelPoint *points, unsigned num_points);
  void DrawPolygon(const BulkPixelPoint *points, unsigned num_points);

  void DrawTriangleFan(const BulkPixelPoint *points, unsigned num_points) {
    DrawPolygon(points, num_points);
  }

  /**
   * Draw a solid thin horizontal line.
   */
  void DrawHLine(int x1, int x2, int y, Color color);

  void DrawLine(int ax, int ay, int bx, int by);

  void DrawLine(const PixelPoint a, const PixelPoint b) {
    DrawLine(a.x, a.y, b.x, b.y);
  }

  void DrawExactLine(int ax, int ay, int bx, int by) {
    DrawLine(ax, ay, bx, by);
  }

  void DrawExactLine(const PixelPoint a, const PixelPoint b) {
    DrawLine(a, b);
  }

  void DrawLinePiece(const PixelPoint a, const PixelPoint b) {
    DrawLine(a, b);
  }

  void DrawTwoLines(int ax, int ay, int bx, int by, int cx, int cy)
  {
    DrawLine(ax, ay, bx, by);
    DrawLine(bx, by, cx, cy);
  }

  void DrawTwoLines(const PixelPoint a, const PixelPoint b,
                    const PixelPoint c) {
    DrawTwoLines(a.x, a.y, b.x, b.y, c.x, c.y);
  }

  void DrawTwoLinesExact(int ax, int ay, int bx, int by, int cx, int cy) {
    DrawTwoLines(ax, ay, bx, by, cx, cy);
  }

  void DrawCircle(int x, int y, unsigned radius);

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

  void DrawFocusRectangle(const PixelRect &rc) {
    DrawOutlineRectangle(rc.left, rc.top, rc.right, rc.bottom,
                         COLOR_DARK_GRAY);
  }

  gcc_pure
  const PixelSize CalcTextSize(TStringView text) const noexcept;

  gcc_pure
  const PixelSize CalcTextSize(const TCHAR *text) const;

  gcc_pure
  unsigned CalcTextWidth(const TCHAR *text) const {
    return CalcTextSize(text).width;
  }

  gcc_pure
  unsigned GetFontHeight() const {
    return font != nullptr ? font->GetHeight() : 0;
  }

  void DrawText(PixelPoint p, const TCHAR *text) noexcept;
  void DrawText(PixelPoint p, const TCHAR *text, size_t length) noexcept;

  void DrawTransparentText(PixelPoint p, const TCHAR *text) noexcept;

  void DrawOpaqueText(PixelPoint p, const PixelRect &rc,
                      const TCHAR *text) noexcept;

  void DrawClippedText(PixelPoint p, const PixelRect &rc,
                       const TCHAR *text) noexcept;
  void DrawClippedText(PixelPoint p, unsigned width,
                       const TCHAR *text) noexcept;

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(PixelPoint p, const TCHAR *t) noexcept {
    DrawText(p, t);
  }

  /**
   * Render multi-line text.
   *
   * @return the resulting text height
   */
  unsigned DrawFormattedText(PixelRect r, const TCHAR *text,
                             unsigned format) noexcept;

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            ConstImageBuffer src, PixelPoint src_position) noexcept;

  void Copy(PixelPoint dest_position, ConstImageBuffer src) noexcept {
    Copy(dest_position, {src.width, src.height}, src, {0, 0});
  }

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            const Canvas &src, PixelPoint src_position) noexcept {
    Copy(dest_position, dest_size, src.buffer, src_position);
  }

  void Copy(const Canvas &src, PixelPoint src_position) noexcept;
  void Copy(const Canvas &src);

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            const Bitmap &src, PixelPoint src_position) noexcept;
  void Copy(const Bitmap &src);

  void CopyTransparentWhite(PixelPoint dest_position, PixelSize dest_size,
                            const Canvas &src,
                            PixelPoint src_position) noexcept;

  void StretchTransparentWhite(PixelPoint dest_position, PixelSize dest_size,
                               ConstImageBuffer src, PixelPoint src_position,
                               PixelSize src_size) noexcept;

  void StretchNot(const Bitmap &src);

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               ConstImageBuffer src,
               PixelPoint src_position, PixelSize src_size) noexcept;

  void Stretch(ConstImageBuffer src) {
    Stretch({0, 0}, GetSize(),
            src, {0, 0}, {src.width, src.height});
  }

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Canvas &src,
               PixelPoint src_position, PixelSize src_size) noexcept {
    Stretch(dest_position, dest_size,
            src.buffer, src_position, src_size);
  }

  void Stretch(const Canvas &src,
               PixelPoint src_position, PixelSize src_size) noexcept;

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src,
               PixelPoint src_position, PixelSize src_size) noexcept;
  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src);

  void Stretch(const Bitmap &src) {
    Stretch({0, 0}, {buffer.width, buffer.height}, src);
  }

  void StretchMono(PixelPoint dest_position, PixelSize dest_size,
                   ::ConstImageBuffer<GreyscalePixelTraits> src,
                   PixelPoint src_position, PixelSize src_size,
                   Color fg_color, Color bg_color);

  void CopyNot(PixelPoint dest_position, PixelSize dest_size,
               ConstImageBuffer src, PixelPoint src_position) noexcept;

  void CopyNot(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src, PixelPoint src_position) noexcept;

  void CopyOr(PixelPoint dest_position, PixelSize dest_size,
              ConstImageBuffer src, PixelPoint src_position) noexcept;

  void CopyOr(PixelPoint dest_position, PixelSize dest_size,
              const Canvas &src, PixelPoint src_position) noexcept {
    CopyOr(dest_position, dest_size, src.buffer, src_position);
  }

  void CopyOr(PixelPoint dest_position, PixelSize dest_size,
              const Bitmap &src, PixelPoint src_position) noexcept;

  void CopyOr(const Bitmap &src) {
    CopyOr({0, 0}, GetSize(), src, {0, 0});
  }

  void CopyNotOr(PixelPoint dest_position, PixelSize dest_size,
                 ConstImageBuffer src, PixelPoint src_position) noexcept;

  void CopyNotOr(PixelPoint dest_position, PixelSize dest_size,
                 const Bitmap &src, PixelPoint src_position) noexcept;

  void CopyAnd(PixelPoint dest_position, PixelSize dest_size,
               ConstImageBuffer src, PixelPoint src_position) noexcept;

  void CopyAnd(PixelPoint dest_position, PixelSize dest_size,
               const Canvas &src, PixelPoint src_position) noexcept {
    CopyAnd(dest_position, dest_size, src.buffer, src_position);
  }

  void CopyAnd(const Canvas &src) {
    CopyAnd({0, 0}, src.GetSize(), src.buffer, {0, 0});
  }

  void CopyAnd(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src, PixelPoint src_position) noexcept;

  void CopyAnd(const Bitmap &src);

  void ScaleCopy(PixelPoint dest_position,
                 const Bitmap &src,
                 PixelPoint src_position, PixelSize src_size) noexcept;

  void AlphaBlend(PixelPoint dest_position, PixelSize dest_size,
                  ConstImageBuffer src,
                  PixelPoint src_position, PixelSize src_size,
                  uint8_t alpha);

  void AlphaBlend(PixelPoint dest_position, PixelSize dest_size,
                  const Canvas &src,
                  PixelPoint src_position, PixelSize src_size,
                  uint8_t alpha);

  /**
   * Like AlphaBlend(), but skip source pixels that are white.
   */
  void AlphaBlendNotWhite(PixelPoint dest_position, PixelSize dest_size,
                          ConstImageBuffer src,
                          PixelPoint src_position, PixelSize src_size,
                          uint8_t alpha);

  void AlphaBlendNotWhite(PixelPoint dest_position, PixelSize dest_size,
                          const Canvas src,
                          PixelPoint src_position, PixelSize src_size,
                          uint8_t alpha);
};

#endif
