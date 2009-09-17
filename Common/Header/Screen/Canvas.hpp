/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#ifndef XCSOAR_SCREEN_CANVAS_HPP
#define XCSOAR_SCREEN_CANVAS_HPP

#include "Screen/Brush.hpp"
#include "Screen/Color.hpp"
#include "Screen/Font.hpp"
#include "Screen/Pen.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <tchar.h>

#ifdef WINDOWSPC
#define HAVE_VIEWPORT
#define HAVE_OFFSET_VIEWPORT
#elif defined(_WIN32_WCE) && _WIN32_WCE >= 0x0500
#define HAVE_VIEWPORT
#if !defined(__GNUC__)
/* OffsetViewportOrgEx() is supported since Windows CE 5.0, but not in
   mingw32ce */
#define HAVE_OFFSET_VIEWPORT
#endif /* __GNUC__ */
#endif

class Canvas {
protected:
  HDC dc;
  unsigned width, height;

private:
  /* copy constructor not allowed */
  Canvas(const Canvas &canvas) {}
  Canvas &operator=(const Canvas &canvas) { return *this; }

public:
  Canvas():dc(NULL) {}
  Canvas(HDC _dc, unsigned _width, unsigned _height)
    :dc(_dc), width(_width), height(_height) {}

  void set(HDC _dc, unsigned _width, unsigned _height) {
    dc = _dc;
    width = _width;
    height = _height;
  }

public:
  bool defined() const {
    return dc != NULL;
  }

  operator HDC() const {
    return dc;
  }

  unsigned get_width() const {
    return width;
  }

  unsigned get_height() const {
    return height;
  }

  void resize(unsigned _width, unsigned _height) {
    width = _width;
    height = _height;
  }

  const HWColor map(const Color color) const
  {
    return HWColor(color);
  }

  void white_pen() {
    ::SelectObject(dc, GetStockObject(WHITE_PEN));
  }

  void black_pen() {
    ::SelectObject(dc, GetStockObject(BLACK_PEN));
  }

  void hollow_brush() {
    ::SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
  }

  void white_brush() {
    ::SelectObject(dc, GetStockObject(WHITE_BRUSH));
  }

  void black_brush() {
    ::SelectObject(dc, GetStockObject(BLACK_BRUSH));
  }

  void select(const Pen &pen) {
    ::SelectObject(dc, pen.native());
  }

  void select(const Brush &brush) {
    ::SelectObject(dc, brush.native());
  }

  void select(const Font &font) {
    ::SelectObject(dc, font.native());
  }

  void set_text_color(const Color c) {
    ::SetTextColor(dc, c);
  }

  void set_background_color(const Color c) {
    ::SetBkColor(dc, c);
  }

  void background_opaque() {
    ::SetBkMode(dc, OPAQUE);
  }

  void background_transparent() {
    ::SetBkMode(dc, TRANSPARENT);
  }

#ifdef HAVE_VIEWPORT
  POINT get_viewport_origin() {
    POINT old;
#ifdef HAVE_OFFSET_VIEWPORT
    ::GetViewportOrgEx(dc, &old);
#else
    ::SetViewportOrgEx(dc, 0, 0, &old);
    if (old.x != 0 || old.y != 0)
      /* restore old viewport */
      ::SetViewportOrgEx(dc, old.x, old.y, NULL);
#endif
    return old;
  }

  POINT set_viewport_origin(int x, int y) {
    POINT old;
    ::SetViewportOrgEx(dc, x, y, &old);
    return old;
  }

#ifdef HAVE_OFFSET_VIEWPORT
  POINT offset_viewport_origin(int dx, int dy) {
    POINT old;
    ::OffsetViewportOrgEx(dc, dx, dy, &old);
    return old;
  }
#endif /* HAVE_OFFSET_VIEWPORT */
#endif /* HAVE_VIEWPORT */

  void rectangle(int left, int top, int right, int bottom) {
    ::Rectangle(dc, left, top, right, bottom);
  }

  void fill_rectangle(const RECT rc, const Brush &brush) {
    ::FillRect(dc, &rc, brush.native());
  }

  void fill_rectangle(int left, int top, int right, int bottom,
                      const Brush &brush) {
    RECT rc;
    rc.left = left;
    rc.top = top;
    rc.right = right;
    rc.bottom = bottom;
    fill_rectangle(rc, brush);
  }

  void clear() {
    rectangle(0, 0, width, height);
  }

  void round_rectangle(int left, int top, int right, int bottom,
                       unsigned ellipse_width, unsigned ellipse_height) {
    ::RoundRect(dc, left, top, right, bottom, ellipse_width, ellipse_height);
  }

  void raised_edge(RECT rc) {
    ::DrawEdge(dc, &rc, EDGE_RAISED, BF_ADJUST | BF_FLAT | BF_RECT);
  }

  void polyline(const POINT* lppt, unsigned cPoints) {
    ::Polyline(dc, lppt, cPoints);
  }

  void polygon(const POINT* lppt, unsigned cPoints) {
    ::Polygon(dc, lppt, cPoints);
  }

  void clipped_polygon(const POINT* lppt, unsigned cPoints, const RECT rc,
                       bool fill=true);
  void clipped_polyline(const POINT* lppt, unsigned cPoints, const RECT rc);
  void clipped_line(const POINT a, const POINT b, const RECT rc);
  void clipped_dashed_line(int width, const POINT a, const POINT b,
                           const Color color, const RECT rc);

  void line(int ax, int ay, int bx, int by);
  void line(const POINT a, const POINT b) {
    line(a.x, a.y, b.x, b.y);
  }

  void two_lines(int ax, int ay, int bx, int by, int cx, int cy);
  void two_lines(const POINT a, const POINT b, const POINT c) {
    two_lines(a.x, a.y, b.x, b.y, c.x, c.y);
  }

  void move_to(int x, int y);
  void line_to(int x, int y);

  void circle(int x, int y, unsigned radius) {
    ::Ellipse(dc, x - radius, y - radius, x + radius, y + radius);
  }

  void circle(int x, int y, unsigned radius,
              const RECT rc, bool clip=false, bool fill=true);

  void arc(int x, int y, unsigned radius, const RECT rc,
           double start, double end);

  void segment(int x, int y, unsigned radius, const RECT rc,
               double start, double end, bool horizon=false);

  void draw_button(RECT rc, bool down) {
    ::DrawFrameControl(dc, &rc, DFC_BUTTON,
                       DFCS_BUTTONPUSH | (down ? DFCS_PUSHED : 0));
  }

  const SIZE text_size(const TCHAR *text, size_t length) const;
  const SIZE text_size(const TCHAR *text) const;
  unsigned text_width(const TCHAR *text) const {
    return text_size(text).cx;
  }

  void text(int x, int y, const TCHAR *text);
  void text_opaque(int x, int y, const TCHAR *text, size_t length);
  void text_opaque(int x, int y, const RECT* lprc, const TCHAR *text);
  void text_opaque(int x, int y, const TCHAR *text) {
    text_opaque(x, y, NULL, text);
  }

  void text_clipped(int x, int y, const RECT &rc, const TCHAR *text);
  void text_clipped(int x, int y, unsigned width, const TCHAR *text);

  void bottom_right_text(int x, int y, const TCHAR *text);

  void formatted_text(RECT *rc, const TCHAR *text, unsigned format) {
    ::DrawText(dc, text, -1, rc, format);
  }

  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            const Canvas &src, int src_x, int src_y);
  void copy(const Canvas &src, int src_x, int src_y);
  void copy(const Canvas &src);

  void copy_transparent_white(const Canvas &src, const RECT &rc);

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void copy_or(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src, int src_x, int src_y);

  void copy_and(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src, int src_x, int src_y);

  void stretch_or(int dest_x, int dest_y,
                  unsigned dest_width, unsigned dest_height,
                  const Canvas &src,
                  int src_x, int src_y,
                  unsigned src_width, unsigned src_height);

  void stretch_and(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   const Canvas &src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height);

  void scale_copy(int dest_x, int dest_y,
                  const Canvas &src,
                  int src_x, int src_y,
                  unsigned src_width, unsigned src_height);

  void scale_or(int dest_x, int dest_y,
                const Canvas &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height);

  void scale_and(int dest_x, int dest_y,
                 const Canvas &src,
                 int src_x, int src_y,
                 unsigned src_width, unsigned src_height);

  void scale_or_and(int dest_x, int dest_y,
                    const Canvas &src,
                    unsigned src_width, unsigned src_height) {
    scale_or(dest_x, dest_y, src, 0, 0, src_width, src_height);
    scale_and(dest_x, dest_y, src, src_width, 0, src_width, src_height);
  }

  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            const Canvas &src, int src_x, int src_y,
            bool transparent)
  {
    if (transparent)
      copy_and(dest_x, dest_y, dest_width, dest_height,
               src, src_x, src_y);
    else
      copy(dest_x, dest_y, dest_width, dest_height,
           src, src_x, src_y);
  }

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height,
               bool transparent)
  {
    if (transparent)
      stretch_and(dest_x, dest_y, dest_width, dest_height,
                  src, src_x, src_y, src_width, src_height);
    else
      stretch(dest_x, dest_y, dest_width, dest_height,
              src, src_x, src_y, src_width, src_height);
  }

  HWColor get_pixel(int x, int y) const {
    return HWColor(::GetPixel(dc, x, y));
  }
};

#endif
