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

#ifndef XCSOAR_SCREEN_SDL_CANVAS_HPP
#define XCSOAR_SCREEN_SDL_CANVAS_HPP

#ifdef ENABLE_OPENGL
#error Please include OpenGL/Canvas.hpp
#endif

#include "Util/NonCopyable.hpp"
#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Font.hpp"
#include "Screen/Pen.hpp"
#include "Screen/SDL/Color.hpp"
#include "Screen/SDL/Point.hpp"
#include "Compiler.h"

#include <assert.h>
#include <windows.h>
#include <tchar.h>

#include <SDL_gfxPrimitives.h>

#ifdef WIN32
/* those are WIN32 macros - undefine, or Canvas::background_mode will
   break */
#undef OPAQUE
#undef TRANSPARENT
#endif

class Bitmap;

/**
 * Base drawable canvas class
 * 
 */
class Canvas : private NonCopyable {
  friend class WindowCanvas;
  friend class SubCanvas;

protected:
  SDL_Surface *surface;

  int x_offset, y_offset;
  unsigned width, height;

  Pen pen;
  Brush brush;
  TTF_Font *font;
  Color text_color, background_color;
  enum {
    OPAQUE, TRANSPARENT
  } background_mode;

public:
  Canvas()
    :surface(NULL), x_offset(0), y_offset(0), width(0), height(0),
     font(NULL), background_mode(OPAQUE) {}
  explicit Canvas(SDL_Surface *_surface)
    :surface(_surface), width(_surface->w), height(_surface->h),
     font(NULL), background_mode(OPAQUE) {}

  void set(SDL_Surface *_surface) {
    reset();

    surface = _surface;
    width = surface->w;
    height = surface->h;
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
    return width;
  }

  unsigned get_height() const {
    return height;
  }

  gcc_pure
  const HWColor map(const Color color) const
  {
    return HWColor(::SDL_MapRGB(surface->format, color.value.r,
                                color.value.g, color.value.b));
  }

  void null_pen() {
    pen = Pen(0, COLOR_BLACK);
  }

  void white_pen() {
    pen = Pen(1, COLOR_WHITE);
  }

  void black_pen() {
    pen = Pen(1, COLOR_BLACK);
  }

  void hollow_brush() {
    brush.reset();
  }

  void white_brush() {
    brush = Brush(COLOR_WHITE);
  }

  void black_brush() {
    brush = Brush(COLOR_BLACK);
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

  void outline_rectangle(int left, int top, int right, int bottom,
                         Color color) {
    ::rectangleColor(surface, left + x_offset, top + y_offset,
                     right + x_offset, bottom + y_offset, color.gfx_color());
  }

  void rectangle(int left, int top, int right, int bottom) {
    fill_rectangle(left, top, right, bottom, brush);

    if (pen_over_brush())
      outline_rectangle(left, top, right, bottom, pen.get_color());
  }

  void fill_rectangle(int left, int top, int right, int bottom,
                      const HWColor color) {
    if (left >= right || top >= bottom)
      return;

    left += x_offset;
    right += x_offset;
    top += y_offset;
    bottom += y_offset;

    SDL_Rect r = { (Sint16)left, (Sint16)top,
                   (Uint16)(right - left), (Uint16)(bottom - top) };
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

  void fill_rectangle(const PixelRect &rc, const HWColor color) {
    fill_rectangle(rc.left, rc.top, rc.right, rc.bottom, color);
  }

  void fill_rectangle(const PixelRect &rc, const Color color) {
    fill_rectangle(rc.left, rc.top, rc.right, rc.bottom, color);
  }

  void fill_rectangle(const PixelRect rc, const Brush &brush) {
    fill_rectangle(rc.left, rc.top, rc.right, rc.bottom, brush);
  }

  void clear() {
    rectangle(0, 0, get_width(), get_height());
  }

  void clear(const HWColor color) {
    fill_rectangle(0, 0, get_width(), get_height(), color);
  }

  void clear(const Color color) {
    fill_rectangle(0, 0, get_width(), get_height(), color);
  }

  void clear(const Brush &brush) {
    fill_rectangle(0, 0, get_width(), get_height(), brush);
  }

  void clear_white() {
    clear(COLOR_WHITE);
  }

  void round_rectangle(int left, int top, int right, int bottom,
                       unsigned ellipse_width, unsigned ellipse_height) {
    rectangle(left, top, right, bottom); // XXX
  }

  void raised_edge(PixelRect &rc) {
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

  void polyline(const RasterPoint *points, unsigned num_points);
  void polygon(const RasterPoint *points, unsigned num_points);

  void line(int ax, int ay, int bx, int by) {
    ax += x_offset;
    bx += x_offset;
    ay += y_offset;
    by += y_offset;

#if SDL_GFXPRIMITIVES_MAJOR > 2 || \
  (SDL_GFXPRIMITIVES_MAJOR == 2 && (SDL_GFXPRIMITIVES_MINOR > 0 || \
                                    SDL_GFXPRIMITIVES_MICRO >= 22))
    /* thickLineColor() was added in SDL_gfx 2.0.22 */
    if (pen.get_width() > 1)
      ::thickLineColor(surface, ax, ay, bx, by,
                       pen.get_width(), pen.get_color().gfx_color());
    else
#endif
      ::lineColor(surface, ax, ay, bx, by, pen.get_color().gfx_color());
  }

  void line(const RasterPoint a, const RasterPoint b) {
    line(a.x, a.y, b.x, b.y);
  }

  void line_piece(const RasterPoint a, const RasterPoint b) {
    line(a.x, a.y, b.x, b.y);
  }

  void two_lines(int ax, int ay, int bx, int by, int cx, int cy)
  {
    line(ax, ay, bx, by);
    line(bx, by, cx, cy);
  }

  void two_lines(const RasterPoint a, const RasterPoint b,
                 const RasterPoint c) {
    two_lines(a.x, a.y, b.x, b.y, c.x, c.y);
  }

  void circle(int x, int y, unsigned radius);

  void segment(int x, int y, unsigned radius,
               Angle start, Angle end, bool horizon=false);

  void annulus(int x, int y, unsigned small_radius, unsigned big_radius,
               Angle start, Angle end);

  void keyhole(int x, int y, unsigned small_radius, unsigned big_radius,
               Angle start, Angle end);

  void draw_focus(PixelRect rc) {
    outline_rectangle(rc.left, rc.top, rc.right, rc.bottom,
                      COLOR_DARK_GRAY);
  }

  void draw_button(PixelRect rc, bool down);

  gcc_pure
  const PixelSize text_size(const TCHAR *text, size_t length) const;

  gcc_pure
  const PixelSize text_size(const TCHAR *text) const;

  gcc_pure
  unsigned text_width(const TCHAR *text) const {
    return text_size(text).cx;
  }

  gcc_pure
  unsigned text_height(const TCHAR *text) const {
    return font != NULL ? TTF_FontHeight(font) : 0;
  }

  void text(int x, int y, const TCHAR *text);
  void text(int x, int y, const TCHAR *text, size_t length);

  void text_transparent(int x, int y, const TCHAR *text);

  void text_opaque(int x, int y, const PixelRect &rc, const TCHAR *text);

  void text_clipped(int x, int y, const PixelRect &rc, const TCHAR *text) {
    // XXX
    this->text(x, y, text);
  }

  void text_clipped(int x, int y, unsigned width, const TCHAR *text) {
    // XXX
    this->text(x, y, text);
  }

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(int x, int y, const TCHAR *t) {
    text(x, y, t);
  }

  void formatted_text(PixelRect *rc, const TCHAR *text, unsigned format);

  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            SDL_Surface *surface, int src_x, int src_y);

  void copy(int dest_x, int dest_y, SDL_Surface *surface) {
    copy(dest_x, dest_y, surface->w, surface->h, surface, 0, 0);
  }

  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            const Canvas &src, int src_x, int src_y) {
    copy(dest_x, dest_y, dest_width, dest_height,
         src.surface, src_x, src_y);
  }

  void copy(const Canvas &src, int src_x, int src_y);
  void copy(const Canvas &src);

  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            const Bitmap &src, int src_x, int src_y);
  void copy(const Bitmap &src);

  void copy_transparent_white(const Canvas &src);
  void copy_transparent_black(const Canvas &src);

  void stretch_transparent(const Bitmap &src, Color key);
  void invert_stretch_transparent(const Bitmap &src, Color key);

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               SDL_Surface *src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(SDL_Surface *src) {
    stretch(0, 0, get_width(), get_height(),
            src, 0, 0, src->w, src->h);
  }

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height) {
    stretch(dest_x, dest_y, dest_width, dest_height,
            src.surface,
            src_x, src_y, src_width, src_height);
  }

  void stretch(const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);
  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src);

  void stretch(const Bitmap &src) {
    stretch(0, 0, width, height, src);
  }

  void copy_not(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                SDL_Surface *src, int src_x, int src_y);

  void copy_not(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src, int src_x, int src_y);

  void copy_or(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               SDL_Surface *src, int src_x, int src_y);

  void copy_or(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src, int src_x, int src_y);

  void copy_or(const Bitmap &src) {
    copy_or(0, 0, get_width(), get_height(), src, 0, 0);
  }

  void copy_and(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                SDL_Surface *src, int src_x, int src_y);

  void copy_and(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src, int src_x, int src_y) {
    copy_and(dest_x, dest_y, dest_width, dest_height,
             src.surface, src_x, src_y);
  }

  void copy_and(const Canvas &src) {
    copy_and(0, 0, src.get_width(), src.get_height(), src, 0, 0);
  }

  void copy_and(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src, int src_x, int src_y);

  void copy_and(const Bitmap &src) {
    copy_and(0, 0, get_width(), get_height(), src, 0, 0);
  }

  void scale_copy(int dest_x, int dest_y,
                  const Bitmap &src,
                  int src_x, int src_y,
                  unsigned src_width, unsigned src_height);
};

#endif
