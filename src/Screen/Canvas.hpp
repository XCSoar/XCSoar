/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Util/NonCopyable.hpp"
#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Color.hpp"
#include "Screen/Font.hpp"
#include "Screen/Pen.hpp"
#include "Compiler.h"

#include <windows.h>
#include <tchar.h>

#ifdef ENABLE_SDL

#include <SDL/SDL_gfxPrimitives.h>

/* those are WIN32 macros - undefine, or Canvas::background_mode will
   break */
#undef OPAQUE
#undef TRANSPARENT

/**
 * Base drawable canvas class
 * 
 */
class Canvas : private NonCopyable {
  friend class WindowCanvas;

protected:
  SDL_Surface *surface;
  Pen pen;
  Brush brush;
  TTF_Font *font;
  Color text_color, background_color;
  enum {
    OPAQUE, TRANSPARENT
  } background_mode;
  POINT cursor;

public:
  Canvas()
    :surface(NULL),
     font(NULL), background_mode(OPAQUE) {}
  explicit Canvas(SDL_Surface *_surface)
    :surface(_surface), font(NULL), background_mode(OPAQUE) {}

  void set(SDL_Surface *_surface) {
    reset();

    surface = _surface;
  }

  void reset();

protected:
  /**
   * Returns true if the outline should be drawn after the area has
   * been filled.  As an optimization, this function returns false if
   * brush and pen share the same color.
   */
  bool pen_over_brush() const {
    return pen.defined() &&
      (brush.is_hollow() || brush.get_color() != pen.get_color());
  }

public:
  bool defined() const {
    return surface != NULL;
  }

  unsigned get_width() const {
    return surface->w;
  }

  unsigned get_height() const {
    return surface->h;
  }

  gcc_pure
  const HWColor map(const Color color) const
  {
    return HWColor(::SDL_MapRGB(surface->format, color.value.r,
                                color.value.g, color.value.b));
  }

  void null_pen() {
    pen = Pen(0, Color::BLACK);
  }

  void white_pen() {
    pen = Pen(1, Color::WHITE);
  }

  void black_pen() {
    pen = Pen(1, Color::BLACK);
  }

  void hollow_brush() {
    brush.reset();
  }

  void white_brush() {
    brush = Brush(Color::WHITE);
  }

  void black_brush() {
    brush = Brush(Color::BLACK);
  }

  void select(const Pen &_pen) {
    pen = _pen;
  }

  void select(const Brush &_brush) {
    brush = _brush;
  }

  void select(const Font &_font) {
    font = _font.native();
  }

  TTF_Font *get_font() const {
    return font;
  }

  void set_text_color(const Color c) {
    text_color = c;
  }

  Color get_text_color() const {
    return text_color;
  }

  void set_background_color(const Color c) {
    background_color = c;
  }

  Color get_background_color() const {
    return background_color;
  }

  void background_opaque() {
    background_mode = OPAQUE;
  }

  void background_transparent() {
    background_mode = TRANSPARENT;
  }

  void mix_copy() {
    // XXX
  }

  void mix_mask() {
    // XXX
  }

  void rectangle(int left, int top, int right, int bottom) {
    fill_rectangle(left, top, right, bottom, brush);

    if (pen_over_brush())
      ::rectangleColor(surface, left, top, right, bottom,
                       pen.get_color().gfx_color());
  }

  void fill_rectangle(int left, int top, int right, int bottom,
                      const HWColor color) {
    if (left >= right || top >= bottom)
      return;

    SDL_Rect r = { left, top, right - left, bottom - top };
    SDL_FillRect(surface, &r, color);
  }

  void fill_rectangle(int left, int top, int right, int bottom,
                      const Color color) {
    fill_rectangle(left, top, right, bottom, map(color));
  }

  void fill_rectangle(int left, int top, int right, int bottom,
                      const Brush &brush) {
    if (brush.is_hollow())
      return;

    fill_rectangle(left, top, right, bottom, brush.get_color());
  }

  void fill_rectangle(const RECT &rc, const HWColor color) {
    fill_rectangle(rc.left, rc.top, rc.right, rc.bottom, color);
  }

  void fill_rectangle(const RECT &rc, const Color color) {
    fill_rectangle(rc.left, rc.top, rc.right, rc.bottom, color);
  }

  void fill_rectangle(const RECT rc, const Brush &brush) {
    fill_rectangle(rc.left, rc.top, rc.right, rc.bottom, brush);
  }

  void clear() {
    rectangle(0, 0, surface->w, surface->h);
  }

  void clear(const HWColor color) {
    fill_rectangle(0, 0, surface->w, surface->h, color);
  }

  void clear(const Color color) {
    fill_rectangle(0, 0, surface->w, surface->h, color);
  }

  void clear(const Brush &brush) {
    fill_rectangle(0, 0, surface->w, surface->h, brush);
  }

  void clear_white() {
    clear(Color::WHITE);
  }

  void round_rectangle(int left, int top, int right, int bottom,
                       unsigned ellipse_width, unsigned ellipse_height) {
    rectangle(left, top, right, bottom); // XXX
  }

  void raised_edge(RECT &rc) {
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

  void polyline(const POINT *lppt, unsigned cPoints) {
    for (unsigned i = 1; i < cPoints; ++i)
      line(lppt[i - 1].x, lppt[i - 1].y, lppt[i].x, lppt[i].y);
  }

  void polygon(const POINT* lppt, unsigned cPoints) {
    if (brush.is_hollow() && !pen.defined())
      return;

    Sint16 vx[cPoints], vy[cPoints];

    for (unsigned i = 0; i < cPoints; ++i) {
      vx[i] = lppt[i].x;
      vy[i] = lppt[i].y;
    }

    if (!brush.is_hollow())
      ::filledPolygonColor(surface, vx, vy, cPoints,
                           brush.get_color().gfx_color());

    if (pen_over_brush())
      ::polygonColor(surface, vx, vy, cPoints, pen.get_color().gfx_color());
  }

  void autoclip_polygon(const POINT* lppt, unsigned cPoints) {
    // XXX clip
    polygon(lppt, cPoints);
  }

  void autoclip_polyline(const POINT* lppt, unsigned cPoints) {
    // XXX clip
    polyline(lppt, cPoints);
  }

  void line(int ax, int ay, int bx, int by) {
    ::lineColor(surface, ax, ay, bx, by, pen.get_color().gfx_color());
  }

  void line(const POINT a, const POINT b) {
    line(a.x, a.y, b.x, b.y);
  }

  void two_lines(int ax, int ay, int bx, int by, int cx, int cy)
  {
    line(ax, ay, bx, by);
    line(bx, by, cx, cy);
  }

  void two_lines(const POINT a, const POINT b, const POINT c) {
    two_lines(a.x, a.y, b.x, b.y, c.x, c.y);
  }

  void move_to(int x, int y);
  void line_to(int x, int y);

  void circle(int x, int y, unsigned radius) {
    if (!brush.is_hollow())
      ::filledCircleColor(surface, x, y, radius,
                          brush.get_color().gfx_color());

    if (pen_over_brush())
      ::circleColor(surface, x, y, radius, pen.get_color().gfx_color());
  }

  void segment(int x, int y, unsigned radius,
               Angle start, Angle end, bool horizon=false);

  void draw_focus(RECT rc) {
    // XXX
  }

  void draw_button(RECT rc, bool down);

  gcc_pure
  const SIZE text_size(const TCHAR *text, size_t length) const;

  gcc_pure
  const SIZE text_size(const TCHAR *text) const;

  gcc_pure
  unsigned text_width(const TCHAR *text) const {
    return text_size(text).cx;
  }

  gcc_pure
  unsigned text_height(const TCHAR *text) const {
    return text_size(_T("W")).cy;
  }

  void text(int x, int y, const TCHAR *text);

  void text(int x, int y, const TCHAR *text, size_t length) {
    // XXX
  }

  void text_opaque(int x, int y, const RECT &rc, const TCHAR *text);

  void text_clipped(int x, int y, const RECT &rc, const TCHAR *text) {
    // XXX
    text_opaque(x, y, rc, text);
  }

  void text_clipped(int x, int y, unsigned width, const TCHAR *text) {
    // XXX
    this->text(x, y, text);
  }

  void bottom_right_text(int x, int y, const TCHAR *text);

  void formatted_text(RECT *rc, const TCHAR *text, unsigned format) {
    // XXX
    this->text(rc->left, rc->top, text);
  }

  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            const Canvas &src, int src_x, int src_y);
  void copy(const Canvas &src, int src_x, int src_y);
  void copy(const Canvas &src);

  void copy_transparent_white(const Canvas &src);
  void copy_transparent_black(const Canvas &src);
  void stretch_transparent(const Canvas &src, Color key);

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(const Canvas &src);

  void copy_or(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src, int src_x, int src_y);

  void copy_or(const Canvas &src) {
    copy_or(0, 0, get_width(), get_height(), src, 0, 0);
  }

  void copy_and(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src, int src_x, int src_y);

  void copy_and(const Canvas &src) {
    copy_and(0, 0, get_width(), get_height(), src, 0, 0);
  }

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

  /**
   * Makes sure the given area is updated on the screen.
   */
  void expose(Sint32 x, Sint32 y, Sint32 w, Sint32 h) {
    ::SDL_UpdateRect(surface, x, y, w, h);
  }

  /**
   * Makes sure the whole area is updated on the screen.
   */
  void expose() {
    expose(0, 0, 0, 0);
  }
};

#else /* !ENABLE_SDL */

#ifndef _WIN32_WCE
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

/**
 * Base drawable canvas class
 * 
 */
class Canvas {
protected:
  HDC dc;
  unsigned width, height;

#ifdef NOLINETO
  POINT cursor;
#endif

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

  gcc_pure
  const HWColor map(const Color color) const
  {
    return HWColor(color);
  }

  void null_pen() {
    ::SelectObject(dc, GetStockObject(NULL_PEN));
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

  gcc_pure
  Color get_text_color() const {
    return Color(::GetTextColor(dc));
  }

  void set_background_color(const Color c) {
    ::SetBkColor(dc, c);
  }

  gcc_pure
  Color get_background_color() const {
    return Color(::GetBkColor(dc));
  }

  void background_opaque() {
    ::SetBkMode(dc, OPAQUE);
  }

  void background_transparent() {
    ::SetBkMode(dc, TRANSPARENT);
  }

  void mix_copy() {
    ::SetROP2(dc, R2_COPYPEN);
  }

  void mix_mask() {
    ::SetROP2(dc, R2_MASKPEN);
  }

#ifdef HAVE_VIEWPORT
  gcc_pure
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

  void fill_rectangle(int left, int top, int right, int bottom,
                      const HWColor color) {
    RECT rc;
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

  void fill_rectangle(const RECT &rc, const HWColor color) {
    /* this hack allows filling a rectangle with a solid color,
       without the need to create a HBRUSH */
    ::SetBkColor(dc, color);
    ::ExtTextOut(dc, rc.left, rc.top, ETO_OPAQUE, &rc, _T(""), 0, NULL);
  }

  void fill_rectangle(const RECT &rc, const Color color) {
    fill_rectangle(rc, map(color));
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
    ::BitBlt(dc, 0, 0, width, height, NULL, 0, 0, WHITENESS);
  }

  void round_rectangle(int left, int top, int right, int bottom,
                       unsigned ellipse_width, unsigned ellipse_height) {
    ::RoundRect(dc, left, top, right, bottom, ellipse_width, ellipse_height);
  }

  void raised_edge(RECT &rc) {
    ::DrawEdge(dc, &rc, EDGE_RAISED, BF_ADJUST | BF_FLAT | BF_RECT);
  }

  void polyline(const POINT* lppt, unsigned cPoints) {
    ::Polyline(dc, lppt, cPoints);
  }

  void polygon(const POINT* lppt, unsigned cPoints) {
    ::Polygon(dc, lppt, cPoints);
  }

private:
  void clipped_polygon(const POINT* lppt, unsigned cPoints);
  void clipped_polyline(const POINT* lppt, unsigned cPoints);

public:
  void autoclip_polygon(const POINT *lppt, unsigned cPoints);
  void autoclip_polyline(const POINT *lppt, unsigned cPoints);

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

  void segment(int x, int y, unsigned radius,
               Angle start, Angle end, bool horizon=false);

  void draw_focus(RECT rc) {
    ::DrawFocusRect(dc, &rc);
  }

  void draw_button(RECT rc, bool down) {
    ::DrawFrameControl(dc, &rc, DFC_BUTTON,
                       DFCS_BUTTONPUSH | (down ? DFCS_PUSHED : 0));
  }

  gcc_pure
  const SIZE text_size(const TCHAR *text, size_t length) const;

  gcc_pure
  const SIZE text_size(const TCHAR *text) const;

  gcc_pure
  unsigned text_width(const TCHAR *text) const {
    return text_size(text).cx;
  }

  gcc_pure
  unsigned text_height(const TCHAR *text) const;

  void text(int x, int y, const TCHAR *text);
  void text(int x, int y, const TCHAR *text, size_t length);
  void text_opaque(int x, int y, const RECT &rc, const TCHAR *text);

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

  void copy_transparent_white(const Canvas &src);
  void copy_transparent_black(const Canvas &src);
  void stretch_transparent(const Canvas &src, Color key);

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(const Canvas &src);

  void copy_or(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src, int src_x, int src_y);

  void copy_or(const Canvas &src) {
    copy_or(0, 0, get_width(), get_height(), src, 0, 0);
  }

  void copy_and(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src, int src_x, int src_y);

  void copy_and(const Canvas &src) {
    copy_and(0, 0, get_width(), get_height(), src, 0, 0);
  }

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

  gcc_pure
  HWColor get_pixel(int x, int y) const {
    return HWColor(::GetPixel(dc, x, y));
  }
};

#endif /* ENABLE_SDL */

#endif
