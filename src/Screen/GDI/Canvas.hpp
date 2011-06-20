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

#ifndef XCSOAR_SCREEN_GDI_CANVAS_HPP
#define XCSOAR_SCREEN_GDI_CANVAS_HPP

#include "Util/NonCopyable.hpp"
#include "Math/Angle.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Color.hpp"
#include "Screen/Font.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Point.hpp"
#include "Screen/GDI/AlphaBlend.hpp"
#include "Compiler.h"

#include <assert.h>
#include <windows.h>
#include <tchar.h>

/**
 * Base drawable canvas class
 * 
 */
class Canvas : private NonCopyable {
protected:
  HDC dc, compatible_dc;
  unsigned width, height;

public:
  Canvas():dc(NULL), compatible_dc(NULL) {}
  Canvas(HDC _dc, unsigned _width, unsigned _height)
    :dc(_dc), compatible_dc(NULL), width(_width), height(_height) {
    assert(dc != NULL);
  }

  ~Canvas() {
    reset();
  }

protected:
  void reset() {
    if (compatible_dc != NULL) {
      ::DeleteDC(compatible_dc);
      compatible_dc = NULL;
    }
  }

  void set(HDC _dc, unsigned _width, unsigned _height) {
    assert(_dc != NULL);
    assert(_width > 0);
    assert(_height > 0);

    reset();

    dc = _dc;
    compatible_dc = NULL;
    width = _width;
    height = _height;
  }

public:
  bool defined() const {
    return dc != NULL;
  }

  operator HDC() const {
    assert(defined());

    return dc;
  }

  unsigned get_width() const {
    assert(defined());

    return width;
  }

  unsigned get_height() const {
    assert(defined());

    return height;
  }

  void resize(unsigned _width, unsigned _height) {
    width = _width;
    height = _height;
  }

  gcc_pure
  const HWColor map(const Color color) const
  {
    return HWColor(color);
  }

  HGDIOBJ select_object(HGDIOBJ handle) {
    assert(defined());
    assert(handle != INVALID_HANDLE_VALUE);

    return ::SelectObject(dc, handle);
  }

  void select_stock(int fnObject) {
    select_object(::GetStockObject(fnObject));
  }

  void null_pen() {
    select_stock(NULL_PEN);
  }

  void white_pen() {
    select_stock(WHITE_PEN);
  }

  void black_pen() {
    select_stock(BLACK_PEN);
  }

  void hollow_brush() {
    select_stock(HOLLOW_BRUSH);
  }

  void white_brush() {
    select_stock(WHITE_BRUSH);
  }

  void black_brush() {
    select_stock(BLACK_BRUSH);
  }

  void select(const Pen &pen) {
    select_object(pen.native());
  }

  void select(const Brush &brush) {
    select_object(brush.native());
  }

  void select(const Font &font) {
    select_object(font.native());
  }

  void set_text_color(const Color c) {
    assert(defined());

    ::SetTextColor(dc, c);
  }

  gcc_pure
  Color get_text_color() const {
    assert(defined());

    return Color(::GetTextColor(dc));
  }

  void set_background_color(const Color c) {
    assert(defined());

    ::SetBkColor(dc, c);
  }

  gcc_pure
  Color get_background_color() const {
    return Color(::GetBkColor(dc));
  }

  void background_opaque() {
    assert(defined());

    ::SetBkMode(dc, OPAQUE);
  }

  void background_transparent() {
    assert(defined());

    ::SetBkMode(dc, TRANSPARENT);
  }

  void mix_copy() {
    assert(defined());

    ::SetROP2(dc, R2_COPYPEN);
  }

  void mix_mask() {
    assert(defined());

    ::SetROP2(dc, R2_MASKPEN);
  }

  void rectangle(int left, int top, int right, int bottom) {
    assert(defined());

    ::Rectangle(dc, left, top, right, bottom);
  }

  void fill_rectangle(int left, int top, int right, int bottom,
                      const HWColor color) {
    PixelRect rc;
    rc.left = left;
    rc.top = top;
    rc.right = right;
    rc.bottom = bottom;

    fill_rectangle(rc, color);
  }

  void fill_rectangle(int left, int top, int right, int bottom,
                      const Color color) {
    fill_rectangle(left, top, right, bottom, map(color));
  }

  void fill_rectangle(const PixelRect &rc, const HWColor color) {
    assert(defined());

    /* this hack allows filling a rectangle with a solid color,
       without the need to create a HBRUSH */
    ::SetBkColor(dc, color);
    ::ExtTextOut(dc, rc.left, rc.top, ETO_OPAQUE, &rc, _T(""), 0, NULL);
  }

  void fill_rectangle(const PixelRect &rc, const Color color) {
    fill_rectangle(rc, map(color));
  }

  void fill_rectangle(const PixelRect rc, const Brush &brush) {
    assert(defined());

    ::FillRect(dc, &rc, brush.native());
  }

  void fill_rectangle(int left, int top, int right, int bottom,
                      const Brush &brush) {
    PixelRect rc;
    rc.left = left;
    rc.top = top;
    rc.right = right;
    rc.bottom = bottom;
    fill_rectangle(rc, brush);
  }

  void clear() {
    rectangle(0, 0, width, height);
  }

  void clear(const HWColor color) {
    fill_rectangle(0, 0, width, height, color);
  }

  void clear(const Color color) {
    fill_rectangle(0, 0, width, height, color);
  }

  void clear(const Brush &brush) {
    fill_rectangle(0, 0, width, height, brush);
  }

  void clear_white() {
    assert(defined());

    ::BitBlt(dc, 0, 0, width, height, NULL, 0, 0, WHITENESS);
  }

  void round_rectangle(int left, int top, int right, int bottom,
                       unsigned ellipse_width, unsigned ellipse_height) {
    assert(defined());

    ::RoundRect(dc, left, top, right, bottom, ellipse_width, ellipse_height);
  }

  void raised_edge(PixelRect &rc) {
    assert(defined());

    ::DrawEdge(dc, &rc, EDGE_RAISED, BF_ADJUST | BF_FLAT | BF_RECT);
  }

  void polyline(const RasterPoint *lppt, unsigned cPoints) {
    assert(defined());

    ::Polyline(dc, lppt, cPoints);
  }

  void polygon(const RasterPoint *lppt, unsigned cPoints) {
    assert(defined());

    ::Polygon(dc, lppt, cPoints);
  }

  void line(int ax, int ay, int bx, int by);
  void line(const RasterPoint a, const RasterPoint b) {
    line(a.x, a.y, b.x, b.y);
  }

  void line_piece(const RasterPoint a, const RasterPoint b) {
    line(a.x, a.y, b.x, b.y);
  }

  void two_lines(int ax, int ay, int bx, int by, int cx, int cy);
  void two_lines(const RasterPoint a, const RasterPoint b,
                 const RasterPoint c) {
    two_lines(a.x, a.y, b.x, b.y, c.x, c.y);
  }

  void circle(int x, int y, unsigned radius) {
    assert(defined());

    ::Ellipse(dc, x - radius, y - radius, x + radius, y + radius);
  }

  void segment(int x, int y, unsigned radius,
               Angle start, Angle end, bool horizon=false);

  void annulus(int x, int y, unsigned small_radius, unsigned big_radius,
               Angle start, Angle end);

  void keyhole(int x, int y, unsigned small_radius, unsigned big_radius,
               Angle start, Angle end);

  void draw_focus(PixelRect rc) {
    assert(defined());

    ::DrawFocusRect(dc, &rc);
  }

  void draw_button(PixelRect rc, bool down) {
    assert(defined());

    ::DrawFrameControl(dc, &rc, DFC_BUTTON,
                       DFCS_BUTTONPUSH | (down ? DFCS_PUSHED : 0));
  }

  gcc_pure
  const PixelSize text_size(const TCHAR *text, size_t length) const;

  gcc_pure
  const PixelSize text_size(const TCHAR *text) const;

  gcc_pure
  unsigned text_width(const TCHAR *text) const {
    return text_size(text).cx;
  }

  gcc_pure
  unsigned text_height(const TCHAR *text) const;

  void text(int x, int y, const TCHAR *text);
  void text(int x, int y, const TCHAR *text, size_t length);
  void text_opaque(int x, int y, const PixelRect &rc, const TCHAR *text);

  void text_clipped(int x, int y, const PixelRect &rc, const TCHAR *text);
  void text_clipped(int x, int y, unsigned width, const TCHAR *text);

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(int x, int y, const TCHAR *t) {
    text(x, y, t);
  }

  void formatted_text(RECT *rc, const TCHAR *text, unsigned format) {
    ::DrawText(dc, text, -1, rc, format);
  }

  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            HDC src, int src_x, int src_y,
            DWORD dwRop=SRCCOPY) {
    assert(defined());
    assert(src != NULL);

    ::BitBlt(dc, dest_x, dest_y, dest_width, dest_height,
             src, src_x, src_y, dwRop);
  }

  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            const Canvas &src, int src_x, int src_y) {
    copy(dest_x, dest_y, dest_width, dest_height,
         src.dc, src_x, src_y);
  }

  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            HBITMAP src, int src_x, int src_y,
            DWORD dwRop=SRCCOPY);

  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            const Bitmap &src, int src_x, int src_y,
            DWORD dwRop=SRCCOPY);

  void copy(const Canvas &src, int src_x, int src_y);
  void copy(const Canvas &src);

  void copy(const Bitmap &src);

  void copy_transparent_white(const Canvas &src);
  void copy_transparent_black(const Canvas &src);
  void stretch_transparent(const Bitmap &src, Color key);
  void invert_stretch_transparent(const Bitmap &src, Color key);

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               HDC src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height) {
    assert(defined());
    assert(src != NULL);

    ::StretchBlt(dc, dest_x, dest_y, dest_width, dest_height,
                 src, src_x, src_y, src_width, src_height,
                 SRCCOPY);
  }

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height) {
    stretch(dest_x, dest_y, dest_width, dest_height,
            src.dc, src_x, src_y, src_width, src_height);
  }

  void stretch(const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(const Canvas &src);

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               HBITMAP src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(const Bitmap &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height) {
    stretch(0, 0, width, height, src, src_x, src_y, src_width, src_height);
  }

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src);

  void stretch(const Bitmap &src);

#ifdef HAVE_ALPHA_BLEND
  void alpha_blend(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   HDC src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height,
                   uint8_t alpha) {
#ifdef HAVE_DYNAMIC_ALPHA_BLEND
    assert(AlphaBlend != NULL);
#endif

    BLENDFUNCTION fn;
    fn.BlendOp = AC_SRC_OVER;
    fn.BlendFlags = 0;
    fn.SourceConstantAlpha = alpha;
    fn.AlphaFormat = 0;

    ::AlphaBlend(dc, dest_x, dest_y, dest_width, dest_height,
                 src, src_x, src_y, src_width, src_height,
                 fn);
  }

  void alpha_blend(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   const Canvas &src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height,
                   uint8_t alpha) {
    alpha_blend(dest_x, dest_y, dest_width, dest_height,
                src.dc, src_x, src_y, src_width, src_height,
                alpha);
  }
#endif

  void copy_or(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src, int src_x, int src_y) {
    copy(dest_x, dest_y, dest_width, dest_height,
         src, src_x, src_y, SRCPAINT);
  }

  void copy_or(const Canvas &src) {
    copy_or(0, 0, get_width(), get_height(), src, 0, 0);
  }

  void copy_or(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src, int src_x, int src_y) {
    copy(dest_x, dest_y, dest_width, dest_height,
         src, src_x, src_y,
         SRCPAINT);
  }

  void copy_not(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src, int src_x, int src_y) {
    copy(dest_x, dest_y, dest_width, dest_height,
         src, src_x, src_y,
         NOTSRCCOPY);
  }

  void copy_and(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src, int src_x, int src_y) {
    copy(dest_x, dest_y, dest_width, dest_height,
         src.dc, src_x, src_y, SRCAND);
  }

  void copy_and(const Canvas &src) {
    copy_and(0, 0, get_width(), get_height(), src, 0, 0);
  }

  void copy_and(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src, int src_x, int src_y) {
    copy(dest_x, dest_y, dest_width, dest_height,
         src, src_x, src_y,
         SRCAND);
  }

  void scale_copy(int dest_x, int dest_y,
                  const Bitmap &src,
                  int src_x, int src_y,
                  unsigned src_width, unsigned src_height);

  gcc_pure
  HWColor get_pixel(int x, int y) const {
    return HWColor(::GetPixel(dc, x, y));
  }
};

#endif
