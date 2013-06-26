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

#ifndef XCSOAR_SCREEN_GDI_CANVAS_HPP
#define XCSOAR_SCREEN_GDI_CANVAS_HPP

#include "Math/Angle.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Color.hpp"
#include "Screen/Font.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Point.hpp"
#include "Compiler.h"

#include <assert.h>
#include <windows.h>
#include <tchar.h>

/**
 * Base drawable canvas class
 * 
 */
class Canvas {
protected:
  HDC dc, compatible_dc;
  PixelSize size;

public:
  Canvas():dc(NULL), compatible_dc(NULL) {}
  Canvas(HDC _dc, PixelSize new_size)
    :dc(_dc), compatible_dc(NULL), size(new_size) {
    assert(dc != NULL);
  }

  ~Canvas() {
    Destroy();
  }

  Canvas(const Canvas &other) = delete;
  Canvas &operator=(const Canvas &other) = delete;

protected:
  void Destroy() {
    if (compatible_dc != NULL) {
      ::DeleteDC(compatible_dc);
      compatible_dc = NULL;
    }
  }

  void Create(HDC _dc, PixelSize new_size) {
    assert(_dc != NULL);
    assert(new_size.cx > 0);
    assert(new_size.cy > 0);

    Destroy();

    dc = _dc;
    compatible_dc = NULL;
    size = new_size;
  }

  HDC GetCompatibleDC() {
    assert(IsDefined());

    if (compatible_dc == NULL)
      compatible_dc = ::CreateCompatibleDC(dc);
    return compatible_dc;
  }

public:
  bool IsDefined() const {
    return dc != NULL;
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

    return size.cx;
  }

  unsigned GetHeight() const {
    assert(IsDefined());

    return size.cy;
  }

  gcc_pure
  PixelRect GetRect() const {
    return PixelRect{RasterPoint{0, 0}, size};
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

  void SelectBlackPen() {
    SelectStockObject(BLACK_PEN);
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

  void Rectangle(int left, int top, int right, int bottom) {
    assert(IsDefined());

    ::Rectangle(dc, left, top, right, bottom);
  }

  void DrawFilledRectangle(int left, int top, int right, int bottom,
                           const HWColor color) {
    PixelRect rc;
    rc.left = left;
    rc.top = top;
    rc.right = right;
    rc.bottom = bottom;

    DrawFilledRectangle(rc, color);
  }

  void DrawFilledRectangle(int left, int top, int right, int bottom,
                           const Color color) {
    DrawFilledRectangle(left, top, right, bottom, map(color));
  }

  void DrawFilledRectangle(const PixelRect &rc, const HWColor color) {
    assert(IsDefined());

    /* this hack allows filling a rectangle with a solid color,
       without the need to create a HBRUSH */
    ::SetBkColor(dc, color);
    ::ExtTextOut(dc, rc.left, rc.top, ETO_OPAQUE, &rc, _T(""), 0, NULL);
  }

  void DrawFilledRectangle(const PixelRect &rc, const Color color) {
    DrawFilledRectangle(rc, map(color));
  }

  void DrawFilledRectangle(const PixelRect &rc, const Brush &brush) {
    assert(IsDefined());

    ::FillRect(dc, &rc, brush.Native());
  }

  void DrawFilledRectangle(int left, int top, int right, int bottom,
                           const Brush &brush) {
    PixelRect rc;
    rc.left = left;
    rc.top = top;
    rc.right = right;
    rc.bottom = bottom;
    DrawFilledRectangle(rc, brush);
  }

  void Clear() {
    Rectangle(0, 0, GetWidth(), GetHeight());
  }

  void Clear(const HWColor color) {
    DrawFilledRectangle(0, 0, GetWidth(), GetHeight(), color);
  }

  void Clear(const Color color) {
    DrawFilledRectangle(0, 0, GetWidth(), GetHeight(), color);
  }

  void Clear(const Brush &brush) {
    DrawFilledRectangle(0, 0, GetWidth(), GetHeight(), brush);
  }

  void ClearWhite() {
    assert(IsDefined());

    ::BitBlt(dc, 0, 0, GetWidth(), GetHeight(), NULL, 0, 0, WHITENESS);
  }

  void DrawRoundRectangle(int left, int top, int right, int bottom,
                          unsigned ellipse_width,
                          unsigned ellipse_height) {
    assert(IsDefined());

    ::RoundRect(dc, left, top, right, bottom, ellipse_width, ellipse_height);
  }

  void DrawRaisedEdge(PixelRect &rc) {
    assert(IsDefined());

    ::DrawEdge(dc, &rc, EDGE_RAISED, BF_ADJUST | BF_RECT);
  }

  void DrawPolyline(const RasterPoint *lppt, unsigned cPoints) {
    assert(IsDefined());

    ::Polyline(dc, lppt, cPoints);
  }

  void DrawPolygon(const RasterPoint *lppt, unsigned cPoints) {
    assert(IsDefined());

    ::Polygon(dc, lppt, cPoints);
  }

  void DrawTriangleFan(const RasterPoint *points, unsigned num_points) {
    DrawPolygon(points, num_points);
  }

  void DrawLine(int ax, int ay, int bx, int by);
  void DrawLine(const RasterPoint a, const RasterPoint b) {
    DrawLine(a.x, a.y, b.x, b.y);
  }

  void DrawLinePiece(const RasterPoint a, const RasterPoint b) {
    DrawLine(a, b);
  }

  void DrawExactLine(int ax, int ay, int bx, int by) {
    DrawLine(ax, ay, bx, by);
  }

  void DrawExactLine(const RasterPoint a, const RasterPoint b) {
    DrawLine(a, b);
  }

  void DrawTwoLines(int ax, int ay, int bx, int by, int cx, int cy);
  void DrawTwoLines(const RasterPoint a, const RasterPoint b,
                    const RasterPoint c) {
    DrawTwoLines(a.x, a.y, b.x, b.y, c.x, c.y);
  }

  void DrawTwoLinesExact(int ax, int ay, int bx, int by, int cx, int cy) {
    DrawTwoLines(ax, ay, bx, by, cx, cy);
  }

  void DrawCircle(int x, int y, unsigned radius) {
    assert(IsDefined());

    ::Ellipse(dc, x - radius, y - radius, x + radius, y + radius);
  }

  void DrawSegment(int x, int y, unsigned radius,
                   Angle start, Angle end, bool horizon = false);

  void DrawAnnulus(int x, int y, unsigned small_radius,
                   unsigned big_radius, Angle start, Angle end);

  void DrawKeyhole(int x, int y, unsigned small_radius,
                   unsigned big_radius, Angle start, Angle end);

  void DrawFocusRectangle(const PixelRect &rc) {
    assert(IsDefined());

    ::DrawFocusRect(dc, &rc);
  }

  void DrawButton(PixelRect rc, bool down) {
    assert(IsDefined());

    ::DrawFrameControl(dc, &rc, DFC_BUTTON,
                       DFCS_BUTTONPUSH | (down ? DFCS_PUSHED : 0));
  }

  gcc_pure
  const PixelSize CalcTextSize(const TCHAR *text, size_t length) const;

  gcc_pure
  const PixelSize CalcTextSize(const TCHAR *text) const;

  gcc_pure
  unsigned CalcTextWidth(const TCHAR *text) const {
    return CalcTextSize(text).cx;
  }

  gcc_pure
  unsigned GetFontHeight() const;

  void DrawText(int x, int y, const TCHAR *text);
  void DrawText(int x, int y,
                const TCHAR *text, size_t length);
  void DrawOpaqueText(int x, int y, const PixelRect &rc, const TCHAR *text);

  void DrawClippedText(int x, int y, const PixelRect &rc,
                       const TCHAR *text);
  void DrawClippedText(int x, int y, unsigned width,
                       const TCHAR *text);

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(int x, int y, const TCHAR *t) {
    DrawText(x, y, t);
  }

  void DrawFormattedText(RECT *rc, const TCHAR *text, unsigned format) {
    ::DrawText(dc, text, -1, rc, format);
  }

  void Copy(int dest_x, int dest_y, unsigned dest_width, unsigned dest_height,
            HDC src, int src_x, int src_y,
            DWORD dwRop=SRCCOPY) {
    assert(IsDefined());
    assert(src != NULL);

    ::BitBlt(dc, dest_x, dest_y, dest_width, dest_height,
             src, src_x, src_y, dwRop);
  }

  void Copy(int dest_x, int dest_y, unsigned dest_width, unsigned dest_height,
            const Canvas &src, int src_x, int src_y) {
    Copy(dest_x, dest_y, dest_width, dest_height,
         src.dc, src_x, src_y);
  }

  void Copy(int dest_x, int dest_y, unsigned dest_width, unsigned dest_height,
            HBITMAP src, int src_x, int src_y,
            DWORD dwRop=SRCCOPY);

  void Copy(int dest_x, int dest_y, unsigned dest_width, unsigned dest_height,
            const Bitmap &src, int src_x, int src_y,
            DWORD dwRop=SRCCOPY);

  void Copy(const Canvas &src, int src_x, int src_y);
  void Copy(const Canvas &src);

  void Copy(const Bitmap &src);

  void CopyTransparentWhite(const Canvas &src);
  void CopyTransparentBlack(const Canvas &src);
  void StretchTransparent(const Bitmap &src, Color key);
  void InvertStretchTransparent(const Bitmap &src, Color key);

  void Stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               HDC src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height,
               DWORD dwRop=SRCCOPY) {
    assert(IsDefined());
    assert(src != NULL);

    ::StretchBlt(dc, dest_x, dest_y, dest_width, dest_height,
                 src, src_x, src_y, src_width, src_height,
                 dwRop);
  }

  void Stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height,
               DWORD dwRop=SRCCOPY) {
    assert(IsDefined());
    assert(src.IsDefined());

    Stretch(dest_x, dest_y, dest_width, dest_height,
            src.dc, src_x, src_y, src_width, src_height,
            dwRop);
  }

  void Stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               HBITMAP src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height,
               DWORD dwRop=SRCCOPY);

  void Stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height,
               DWORD dwRop=SRCCOPY);

  void Stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               HDC src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height) {
    assert(IsDefined());
    assert(src != NULL);

    Stretch(dest_x, dest_y, dest_width, dest_height,
            src, src_x, src_y, src_width, src_height,
            SRCCOPY);
  }

  void Stretch(const Canvas &src,
               int src_x, int src_y, unsigned src_width, unsigned src_height);

  void Stretch(const Canvas &src);

  void Stretch(const Bitmap &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height) {
    Stretch(0, 0, GetWidth(), GetHeight(),
            src, src_x, src_y, src_width, src_height);
  }

  void Stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src);

  void Stretch(const Bitmap &src);

  void StretchAnd(int dest_x, int dest_y,
                  unsigned dest_width, unsigned dest_height,
                  const Bitmap &src,
                  int src_x, int src_y,
                  unsigned src_width, unsigned src_height) {
    Stretch(dest_x, dest_y, dest_width, dest_height,
            src, src_x, src_y, src_width, src_height,
            SRCAND);
  }

  void StretchNotOr(int dest_x, int dest_y,
                    unsigned dest_width, unsigned dest_height,
                    const Bitmap &src,
                    int src_x, int src_y,
                    unsigned src_width, unsigned src_height) {
    Stretch(dest_x, dest_y, dest_width, dest_height,
            src, src_x, src_y, src_width, src_height,
            MERGEPAINT);
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
  void StretchMono(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   const Bitmap &src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height,
                   Color fg_color, Color bg_color);

#ifdef HAVE_ALPHA_BLEND
  void AlphaBlend(int dest_x, int dest_y,
                  unsigned dest_width, unsigned dest_height,
                  HDC src,
                  int src_x, int src_y,
                  unsigned src_width, unsigned src_height,
                  uint8_t alpha);

  void AlphaBlend(int dest_x, int dest_y,
                  unsigned dest_width, unsigned dest_height,
                  const Canvas &src,
                  int src_x, int src_y,
                  unsigned src_width, unsigned src_height,
                  uint8_t alpha) {
    AlphaBlend(dest_x, dest_y, dest_width, dest_height,
                src.dc, src_x, src_y, src_width, src_height,
                alpha);
  }
#endif

  void CopyOr(int dest_x, int dest_y,
              unsigned dest_width, unsigned dest_height,
              const Canvas &src, int src_x, int src_y) {
    Copy(dest_x, dest_y, dest_width, dest_height,
         src, src_x, src_y, SRCPAINT);
  }

  void CopyOr(const Canvas &src) {
    CopyOr(0, 0, GetWidth(), GetHeight(), src, 0, 0);
  }

  void CopyOr(int dest_x, int dest_y,
              unsigned dest_width, unsigned dest_height,
              const Bitmap &src, int src_x, int src_y) {
    Copy(dest_x, dest_y, dest_width, dest_height,
         src, src_x, src_y,
         SRCPAINT);
  }

  /**
   * Merges the colors of the inverted source bitmap with the colors
   * of this Canvas using the "OR" operator.
   */
  void CopyNotOr(int dest_x, int dest_y,
                 unsigned dest_width, unsigned dest_height,
                 const Bitmap &src, int src_x, int src_y) {
    Copy(dest_x, dest_y, dest_width, dest_height,
         src, src_x, src_y,
         MERGEPAINT);
  }

  void CopyNot(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src, int src_x, int src_y) {
    Copy(dest_x, dest_y, dest_width, dest_height,
         src, src_x, src_y,
         NOTSRCCOPY);
  }

  void CopyAnd(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src, int src_x, int src_y) {
    Copy(dest_x, dest_y, dest_width, dest_height,
         src.dc, src_x, src_y, SRCAND);
  }

  void CopyAnd(const Canvas &src) {
    CopyAnd(0, 0, GetWidth(), GetHeight(), src, 0, 0);
  }

  void CopyAnd(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src, int src_x, int src_y) {
    Copy(dest_x, dest_y, dest_width, dest_height,
         src, src_x, src_y,
         SRCAND);
  }

  void ScaleCopy(int dest_x, int dest_y,
                 const Bitmap &src,
                 int src_x, int src_y,
                 unsigned src_width, unsigned src_height);

  gcc_pure
  HWColor GetPixel(int x, int y) const {
    return HWColor(::GetPixel(dc, x, y));
  }
};

#endif
