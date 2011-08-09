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

protected:
  int x_offset, y_offset;
  unsigned width, height;

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
  Canvas(unsigned _width, unsigned _height)
    :width(_width), height(_height),
     font(NULL), background_mode(OPAQUE) {}

  void set(unsigned _width, unsigned _height) {
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
    return pen.defined() &&
      (brush.is_hollow() || brush.get_color() != pen.get_color());
  }

public:
  bool defined() const {
    return true;
  }

  unsigned get_width() const {
    return width;
  }

  unsigned get_height() const {
    return height;
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
    font = &_font;
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

  void rectangle(int left, int top, int right, int bottom) {
    fill_rectangle(left, top, right, bottom, brush);

    if (pen_over_brush())
      outline_rectangle(left, top, right, bottom);
  }

  void fill_rectangle(int left, int top, int right, int bottom,
                      const Color color);

  void fill_rectangle(int left, int top, int right, int bottom,
                      const Brush &brush) {
    if (!brush.is_hollow())
      fill_rectangle(left, top, right, bottom, brush.get_color());
  }

  void fill_rectangle(const PixelRect &rc, const Color color) {
    fill_rectangle(rc.left, rc.top, rc.right, rc.bottom, color);
  }

  void fill_rectangle(const PixelRect rc, const Brush &brush) {
    fill_rectangle(rc.left, rc.top, rc.right, rc.bottom, brush);
  }

  /**
   * Draw a rectangle outline with the current OpenGL color and
   * settings.
   */
  void OutlineRectangleGL(int left, int top, int right, int bottom);

  void outline_rectangle(int left, int top, int right, int bottom) {
    pen.set();
    OutlineRectangleGL(left, top, right, bottom);
  }

  void outline_rectangle(int left, int top, int right, int bottom,
                         Color color) {
    color.set();
#ifdef HAVE_GLES
    glLineWidthx(1 << 16);
#else
    glLineWidth(1);
#endif

    OutlineRectangleGL(left, top, right, bottom);
  }

  void clear() {
    rectangle(0, 0, get_width(), get_height());
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

  void raised_edge(PixelRect &rc);

  void polyline(const RasterPoint *points, unsigned num_points);

  void polygon(const RasterPoint *points, unsigned num_points);

  void line(int ax, int ay, int bx, int by);

  void line(const RasterPoint a, const RasterPoint b) {
    line(a.x, a.y, b.x, b.y);
  }

  void line_piece(const RasterPoint a, const RasterPoint b);

  void two_lines(int ax, int ay, int bx, int by, int cx, int cy);
  void two_lines(const RasterPoint a, const RasterPoint b,
                 const RasterPoint c);

  void circle(int x, int y, unsigned radius);

  void segment(int x, int y, unsigned radius,
               Angle start, Angle end, bool horizon=false);

  void annulus(int x, int y, unsigned small_radius, unsigned big_radius,
               Angle start, Angle end);

  void keyhole(int x, int y, unsigned small_radius, unsigned big_radius,
               Angle start, Angle end);

  void draw_focus(PixelRect rc);

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
    return font != NULL ? font->get_height() : 0;
  }

  void text(int x, int y, const TCHAR *text);
  void text(int x, int y, const TCHAR *text, size_t length);

  void text_transparent(int x, int y, const TCHAR *text);

  void text_opaque(int x, int y, const PixelRect &rc, const TCHAR *text);

  void text_clipped(int x, int y, const PixelRect &rc, const TCHAR *text) {
    // XXX

    if (x < rc.right)
      text_clipped(x, y, rc.right - x, text);
  }

  void text_clipped(int x, int y, unsigned width, const TCHAR *text);

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(int x, int y, const TCHAR *t) {
    if (x < (int)get_width())
      text_clipped(x, y, get_width() - x, t);
  }

  void formatted_text(PixelRect *rc, const TCHAR *text, unsigned format);

  /**
   * Draws a texture.  The caller is responsible for binding it and
   * enabling GL_TEXTURE_2D.
   */
  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const GLTexture &texture,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);

  void stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const GLTexture &texture);


  void copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            const Bitmap &src, int src_x, int src_y);
  void copy(const Bitmap &src);

  void stretch_transparent(const Bitmap &src, Color key);
  void invert_stretch_transparent(const Bitmap &src, Color key);

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

  void copy_or(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src, int src_x, int src_y);

  void copy_or(const Bitmap &src) {
    copy_or(0, 0, get_width(), get_height(), src, 0, 0);
  }

  void copy_not(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src, int src_x, int src_y);

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
