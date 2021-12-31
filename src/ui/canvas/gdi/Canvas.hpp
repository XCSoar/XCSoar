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

#ifndef XCSOAR_SCREEN_GDI_CANVAS_HPP
#define XCSOAR_SCREEN_GDI_CANVAS_HPP

#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "util/Compiler.h"
#include "util/StringView.hxx"

#include <cassert>

#include <handleapi.h> // for INVALID_HANDLE_VALUE
#include <tchar.h>
#include <wingdi.h>
#include <winuser.h>

class Angle;

/**
 * Base drawable canvas class
 */
class Canvas {
protected:
  HDC dc = nullptr, compatible_dc = nullptr;
  PixelSize size;

public:
  Canvas() = default;
  Canvas(HDC _dc, PixelSize new_size)
    :dc(_dc), size(new_size) {
    assert(dc != nullptr);
  }

  ~Canvas() {
    Destroy();
  }

  Canvas(const Canvas &other) = delete;
  Canvas &operator=(const Canvas &other) = delete;

protected:
  void Destroy() {
    if (compatible_dc != nullptr) {
      ::DeleteDC(compatible_dc);
      compatible_dc = nullptr;
    }
  }

  void Create(HDC _dc, PixelSize new_size) {
    assert(_dc != nullptr);
    assert(new_size.width > 0);
    assert(new_size.height > 0);

    Destroy();

    dc = _dc;
    compatible_dc = nullptr;
    size = new_size;
  }

  HDC GetCompatibleDC() {
    assert(IsDefined());

    if (compatible_dc == nullptr)
      compatible_dc = ::CreateCompatibleDC(dc);
    return compatible_dc;
  }

public:
  bool IsDefined() const {
    return dc != nullptr;
  }

  operator HDC() const {
    assert(IsDefined());

    return dc;
  }

  const PixelSize &GetSize() const {
    return size;
  }

  unsigned GetWidth() const {
    assert(IsDefined());

    return size.width;
  }

  unsigned GetHeight() const {
    assert(IsDefined());

    return size.height;
  }

  gcc_pure
  PixelRect GetRect() const {
    return PixelRect{PixelPoint{0, 0}, size};
  }

  void Resize(PixelSize new_size) {
    size = new_size;
  }

  gcc_pure
  const HWColor map(const Color color) const
  {
    return HWColor(color);
  }

  HGDIOBJ SelectObject(HGDIOBJ handle) {
    assert(IsDefined());
    assert(handle != INVALID_HANDLE_VALUE);

    return ::SelectObject(dc, handle);
  }

  void SelectStockObject(int fnObject) {
    SelectObject(::GetStockObject(fnObject));
  }

  void SelectNullPen() {
    SelectStockObject(NULL_PEN);
  }

  void SelectWhitePen() {
    SelectStockObject(WHITE_PEN);
  }

  void SelectWhitePen(unsigned width) {
    Select(Pen(width, COLOR_WHITE));
  }

  void SelectBlackPen() {
    SelectStockObject(BLACK_PEN);
  }

  void SelectBlackPen(unsigned width) {
    Select(Pen(width, COLOR_BLACK));
  }

  void SelectHollowBrush() {
    SelectStockObject(HOLLOW_BRUSH);
  }

  void SelectWhiteBrush() {
    SelectStockObject(WHITE_BRUSH);
  }

  void SelectBlackBrush() {
    SelectStockObject(BLACK_BRUSH);
  }

  void Select(const Pen &pen) {
    SelectObject(pen.Native());
  }

  void Select(const Brush &brush) {
    SelectObject(brush.Native());
  }

  void Select(const Font &font) {
    SelectObject(font.Native());
  }

  void SetTextColor(const Color c) {
    assert(IsDefined());

    ::SetTextColor(dc, c);
  }

  gcc_pure
  Color GetTextColor() const {
    assert(IsDefined());

    return Color(::GetTextColor(dc));
  }

  void SetBackgroundColor(const Color c) {
    assert(IsDefined());

    ::SetBkColor(dc, c);
  }

  gcc_pure
  Color GetBackgroundColor() const {
    return Color(::GetBkColor(dc));
  }

  void SetBackgroundOpaque() {
    assert(IsDefined());

    ::SetBkMode(dc, OPAQUE);
  }

  void SetBackgroundTransparent() {
    assert(IsDefined());

    ::SetBkMode(dc, TRANSPARENT);
  }

  void SetMixCopy() {
    assert(IsDefined());

    ::SetROP2(dc, R2_COPYPEN);
  }

  void SetMixMask() {
    assert(IsDefined());

    ::SetROP2(dc, R2_MASKPEN);
  }

  void DrawRectangle(PixelRect r) noexcept {
    assert(IsDefined());

    ::Rectangle(dc, r.left, r.top, r.right, r.bottom);
  }

  void DrawFilledRectangle(const PixelRect &_rc, const HWColor color) {
    assert(IsDefined());

    const RECT rc = _rc;

    /* this hack allows filling a rectangle with a solid color,
       without the need to create a HBRUSH */
    ::SetBkColor(dc, color);
    ::ExtTextOut(dc, rc.left, rc.top, ETO_OPAQUE, &rc, _T(""), 0, nullptr);
  }

  void DrawFilledRectangle(const PixelRect &rc, const Color color) {
    DrawFilledRectangle(rc, map(color));
  }

  void DrawFilledRectangle(const PixelRect &_rc, const Brush &brush) {
    assert(IsDefined());

    const RECT rc = _rc;
    ::FillRect(dc, &rc, brush.Native());
  }

  void InvertRectangle(const RECT r) {
    ::InvertRect(dc, &r);
  }

  void InvertRectangle(const PixelRect &rc) {
    InvertRectangle((RECT)rc);
  }

  void Clear() {
    DrawRectangle(GetRect());
  }

  void Clear(const HWColor color) {
    DrawFilledRectangle(GetRect(), color);
  }

  void Clear(const Color color) {
    DrawFilledRectangle(GetRect(), color);
  }

  void Clear(const Brush &brush) {
    DrawFilledRectangle(GetRect(), brush);
  }

  void ClearWhite() {
    assert(IsDefined());

    ::BitBlt(dc, 0, 0, GetWidth(), GetHeight(), nullptr, 0, 0, WHITENESS);
  }

  void DrawRoundRectangle(PixelRect r, PixelSize ellipse_size) noexcept {
    assert(IsDefined());

    ::RoundRect(dc, r.left, r.top, r.right, r.bottom,
                ellipse_size.width, ellipse_size.height);
  }

  void DrawRaisedEdge(PixelRect &_rc) {
    assert(IsDefined());

    RECT rc = _rc;
    ::DrawEdge(dc, &rc, EDGE_RAISED, BF_ADJUST | BF_RECT);
    _rc = rc;
  }

  void DrawPolyline(const BulkPixelPoint *lppt, unsigned cPoints) {
    assert(IsDefined());

    ::Polyline(dc, lppt, cPoints);
  }

  void DrawPolygon(const BulkPixelPoint *lppt, unsigned cPoints) {
    assert(IsDefined());

    ::Polygon(dc, lppt, cPoints);
  }

  void DrawTriangleFan(const BulkPixelPoint *points, unsigned num_points) {
    DrawPolygon(points, num_points);
  }

  void DrawLine(int ax, int ay, int bx, int by);
  void DrawLine(const PixelPoint a, const PixelPoint b) {
    DrawLine(a.x, a.y, b.x, b.y);
  }

  void DrawLinePiece(const PixelPoint a, const PixelPoint b) {
    DrawLine(a, b);
  }

  void DrawExactLine(int ax, int ay, int bx, int by) {
    DrawLine(ax, ay, bx, by);
  }

  void DrawExactLine(const PixelPoint a, const PixelPoint b) {
    DrawLine(a, b);
  }

  void DrawTwoLines(int ax, int ay, int bx, int by, int cx, int cy);
  void DrawTwoLines(const PixelPoint a, const PixelPoint b,
                    const PixelPoint c) {
    DrawTwoLines(a.x, a.y, b.x, b.y, c.x, c.y);
  }

  void DrawTwoLinesExact(PixelPoint a, PixelPoint b, PixelPoint c) noexcept {
    DrawTwoLines(a, b, c);
  }

  void DrawCircle(PixelPoint center, unsigned radius) noexcept {
    assert(IsDefined());

    ::Ellipse(dc, center.x - radius, center.y - radius,
              center.x + radius, center.y + radius);
  }

  void DrawSegment(PixelPoint center, unsigned radius,
                   Angle start, Angle end, bool horizon = false);

  void DrawAnnulus(PixelPoint center, unsigned small_radius,
                   unsigned big_radius, Angle start, Angle end);

  void DrawKeyhole(PixelPoint center, unsigned small_radius,
                   unsigned big_radius, Angle start, Angle end);

  void DrawArc(PixelPoint center, unsigned radius,
               Angle start, Angle end);

  void DrawFocusRectangle(const PixelRect &_rc) {
    assert(IsDefined());

    const RECT rc = _rc;
    ::DrawFocusRect(dc, &rc);
  }

  gcc_pure
  const PixelSize CalcTextSize(BasicStringView<TCHAR> text) const noexcept;

  gcc_pure
  unsigned CalcTextWidth(BasicStringView<TCHAR> text) const {
    return CalcTextSize(text).width;
  }

  gcc_pure
  unsigned GetFontHeight() const;

  void DrawText(PixelPoint p, BasicStringView<TCHAR> text) noexcept;
  void DrawOpaqueText(PixelPoint p, const PixelRect &rc,
                      BasicStringView<TCHAR> text) noexcept;

  void DrawClippedText(PixelPoint p, const PixelRect &rc,
                       BasicStringView<TCHAR> text) noexcept;
  void DrawClippedText(PixelPoint p, unsigned width,
                       BasicStringView<TCHAR> text) noexcept;

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(PixelPoint p, BasicStringView<TCHAR> t) noexcept {
    DrawText(p, t);
  }

  unsigned DrawFormattedText(RECT rc, BasicStringView<TCHAR> text, unsigned format) {
    format |= DT_NOPREFIX | DT_WORDBREAK;
    ::DrawText(dc, text.data, text.size, &rc, format);
    return rc.bottom - rc.top;
  }

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            HDC src, PixelPoint src_position,
            DWORD dwRop=SRCCOPY) {
    assert(IsDefined());
    assert(src != nullptr);

    ::BitBlt(dc, dest_position.x, dest_position.y,
             dest_size.width, dest_size.height,
             src, src_position.x, src_position.y, dwRop);
  }

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            const Canvas &src, PixelPoint src_position) noexcept {
    Copy(dest_position, dest_size, src.dc, src_position);
  }

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            HBITMAP src, PixelPoint src_position,
            DWORD dwRop=SRCCOPY);

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            const Bitmap &src, PixelPoint src_position,
            DWORD dwRop=SRCCOPY);

  void Copy(const Canvas &src, PixelPoint src_position) noexcept;
  void Copy(const Canvas &src);

  void Copy(const Bitmap &src);

  void CopyTransparentWhite(PixelPoint dest_position, PixelSize dest_size,
                            const Canvas &src,
                            PixelPoint src_position) noexcept;

  void StretchNot(const Bitmap &src);

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               HDC src, PixelPoint src_position, PixelSize src_size,
               DWORD dwRop=SRCCOPY) {
    assert(IsDefined());
    assert(src != nullptr);

    ::StretchBlt(dc, dest_position.x, dest_position.y,
                 dest_size.width, dest_size.height,
                 src, src_position.x, src_position.y,
                 src_size.width, src_size.height,
                 dwRop);
  }

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Canvas &src,
               PixelPoint src_position, PixelSize src_size,
               DWORD dwRop=SRCCOPY) {
    assert(IsDefined());
    assert(src.IsDefined());

    Stretch(dest_position, dest_size,
            src.dc, src_position, src_size,
            dwRop);
  }

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               HBITMAP src, PixelPoint src_position, PixelSize src_size,
               DWORD dwRop=SRCCOPY);

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src,
               PixelPoint src_position, PixelSize src_size,
               DWORD dwRop=SRCCOPY);

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               HDC src,
               PixelPoint src_position, PixelSize src_size) noexcept {
    assert(IsDefined());
    assert(src != nullptr);

    Stretch(dest_position, dest_size,
            src, src_position, src_size,
            SRCCOPY);
  }

  void Stretch(const Canvas &src,
               PixelPoint src_position, PixelSize src_size) noexcept;

  void Stretch(const Canvas &src);

  void Stretch(const Bitmap &src,
               PixelPoint src_position, PixelSize src_size) noexcept {
    Stretch({0, 0}, GetSize(),
            src, src_position, src_size);
  }

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src);

  void Stretch(const Bitmap &src);

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

#ifdef HAVE_ALPHA_BLEND
  void AlphaBlend(PixelPoint dest_position, PixelSize dest_size,
                  HDC src, PixelPoint src_position, PixelSize src_size,
                  uint8_t alpha);

  void AlphaBlend(PixelPoint dest_position, PixelSize dest_size,
                  const Canvas &src,
                  PixelPoint src_position, PixelSize src_size,
                  uint8_t alpha) {
    AlphaBlend(dest_position, dest_size,
               src.dc, src_position, src_size,
               alpha);
  }
#endif

  void CopyOr(PixelPoint dest_position, PixelSize dest_size,
              const Canvas &src, PixelPoint src_position) noexcept {
    Copy(dest_position, dest_size, src, src_position, SRCPAINT);
  }

  void CopyOr(const Canvas &src) {
    CopyOr({0, 0}, GetSize(), src, {0, 0});
  }

  void CopyOr(PixelPoint dest_position, PixelSize dest_size,
              const Bitmap &src, PixelPoint src_position) noexcept {
    Copy(dest_position, dest_size, src, src_position, SRCPAINT);
  }

  /**
   * Merges the colors of the inverted source bitmap with the colors
   * of this Canvas using the "OR" operator.
   */
  void CopyNotOr(PixelPoint dest_position, PixelSize dest_size,
                 const Bitmap &src, PixelPoint src_position) noexcept {
    Copy(dest_position, dest_size, src, src_position, MERGEPAINT);
  }

  void CopyNot(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src, PixelPoint src_position) noexcept {
    Copy(dest_position, dest_size, src, src_position, NOTSRCCOPY);
  }

  void CopyAnd(PixelPoint dest_position, PixelSize dest_size,
               const Canvas &src, PixelPoint src_position) noexcept {
    Copy(dest_position, dest_size, src.dc, src_position, SRCAND);
  }

  void CopyAnd(const Canvas &src) {
    CopyAnd({0, 0}, GetSize(), src, {0, 0});
  }

  void CopyAnd(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src, PixelPoint src_position) noexcept {
    Copy(dest_position, dest_size, src, src_position, SRCAND);
  }

  void ScaleCopy(PixelPoint dest_position,
                 const Bitmap &src,
                 PixelPoint src_position, PixelSize src_size) noexcept;
};

#endif
