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

#ifndef XCSOAR_SCREEN_SDL_CANVAS_HPP
#define XCSOAR_SCREEN_SDL_CANVAS_HPP

#ifdef ENABLE_OPENGL
#error Please include OpenGL/Canvas.hpp
#endif

#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Font.hpp"
#include "Screen/Pen.hpp"
#include "Screen/SDL/Color.hpp"
#include "Screen/SDL/Point.hpp"
#include "Compiler.h"

#include <assert.h>
#include <tchar.h>

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
class Canvas {
  friend class WindowCanvas;
  friend class SubCanvas;

protected:
  SDL_Surface *surface;

  RasterPoint offset;
  PixelSize size;

  Pen pen;
  Brush brush;
  const Font *font;
  Color text_color, background_color;
  enum {
    OPAQUE, TRANSPARENT
  } background_mode;

public:
  Canvas()
    :surface(NULL), offset(0, 0), size(0, 0),
     font(NULL), background_mode(OPAQUE) {}
  explicit Canvas(SDL_Surface *_surface)
    :surface(_surface), offset(0, 0), size(_surface->w, _surface->h),
     font(NULL), background_mode(OPAQUE) {}

  Canvas(const Canvas &other) = delete;
  Canvas &operator=(const Canvas &other) = delete;

  void Create(SDL_Surface *_surface) {
    Destroy();

    surface = _surface;
    size = { surface->w, surface->h };
  }

  void Destroy();

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
    return surface != NULL;
  }

  PixelSize GetSize() const {
    return size;
  }

  unsigned GetWidth() const {
    return size.cx;
  }

  unsigned GetHeight() const {
    return size.cy;
  }

  gcc_pure
  PixelRect GetRect() const {
    return PixelRect(size);
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
              rc.right - 2, rc.top);

    Pen dark(1, Color(128, 128, 128));
    Select(dark);
    DrawTwoLines(rc.left + 1, rc.bottom - 1, rc.right - 1, rc.bottom - 1,
              rc.right - 1, rc.top + 1);

    ++rc.left;
    ++rc.top;
    --rc.right;
    --rc.bottom;
  }

  void DrawPolyline(const RasterPoint *points, unsigned num_points);
  void DrawPolygon(const RasterPoint *points, unsigned num_points);

  void DrawTriangleFan(const RasterPoint *points, unsigned num_points) {
    DrawPolygon(points, num_points);
  }

  void DrawLine(int ax, int ay, int bx, int by);

  void DrawLine(const RasterPoint a, const RasterPoint b) {
    DrawLine(a.x, a.y, b.x, b.y);
  }

  void DrawExactLine(int ax, int ay, int bx, int by) {
    DrawLine(ax, ay, bx, by);
  }

  void DrawExactLine(const RasterPoint a, const RasterPoint b) {
    DrawLine(a, b);
  }

  void DrawLinePiece(const RasterPoint a, const RasterPoint b) {
    DrawLine(a, b);
  }

  void DrawTwoLines(int ax, int ay, int bx, int by, int cx, int cy)
  {
    DrawLine(ax, ay, bx, by);
    DrawLine(bx, by, cx, cy);
  }

  void DrawTwoLines(const RasterPoint a, const RasterPoint b,
                 const RasterPoint c) {
    DrawTwoLines(a.x, a.y, b.x, b.y, c.x, c.y);
  }

  void DrawTwoLinesExact(int ax, int ay, int bx, int by, int cx, int cy) {
    DrawTwoLines(ax, ay, bx, by, cx, cy);
  }

  void DrawCircle(int x, int y, unsigned radius);

  void DrawSegment(int x, int y, unsigned radius,
                   Angle start, Angle end, bool horizon=false);

  void DrawAnnulus(int x, int y, unsigned small_radius, unsigned big_radius,
                   Angle start, Angle end);

  void DrawKeyhole(int x, int y,
                   unsigned small_radius, unsigned big_radius,
                   Angle start, Angle end);

  void DrawFocusRectangle(const PixelRect &rc) {
    DrawOutlineRectangle(rc.left, rc.top, rc.right, rc.bottom,
                         COLOR_DARK_GRAY);
  }

  void DrawButton(PixelRect rc, bool down);

  gcc_pure
  const PixelSize CalcTextSize(const TCHAR *text, size_t length) const;

  gcc_pure
  const PixelSize CalcTextSize(const TCHAR *text) const;

  gcc_pure
  unsigned CalcTextWidth(const TCHAR *text) const {
    return CalcTextSize(text).cx;
  }

  gcc_pure
  unsigned GetFontHeight() const {
    return font != NULL ? font->GetHeight() : 0;
  }

  void DrawText(int x, int y, const TCHAR *text);
  void DrawText(int x, int y,
                const TCHAR *text, size_t length);

  void DrawTransparentText(int x, int y, const TCHAR *text);

  void DrawOpaqueText(int x, int y, const PixelRect &rc, const TCHAR *text);

  void DrawClippedText(int x, int y, const PixelRect &rc, const TCHAR *text) {
    // XXX
    DrawText(x, y, text);
  }

  void DrawClippedText(int x, int y, unsigned width, const TCHAR *text) {
    // XXX
    DrawText(x, y, text);
  }

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(int x, int y, const TCHAR *t) {
    DrawText(x, y, t);
  }

  void DrawFormattedText(PixelRect *rc, const TCHAR *text, unsigned format);

  void Copy(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            SDL_Surface *surface, int src_x, int src_y);

  void Copy(int dest_x, int dest_y, SDL_Surface *surface) {
    Copy(dest_x, dest_y, surface->w, surface->h, surface, 0, 0);
  }

  void Copy(int dest_x, int dest_y, unsigned dest_width, unsigned dest_height,
            const Canvas &src, int src_x, int src_y) {
    Copy(dest_x, dest_y, dest_width, dest_height,
         src.surface, src_x, src_y);
  }

  void Copy(const Canvas &src, int src_x, int src_y);
  void Copy(const Canvas &src);

  void Copy(int dest_x, int dest_y, unsigned dest_width, unsigned dest_height,
            const Bitmap &src, int src_x, int src_y);
  void Copy(const Bitmap &src);

  void CopyTransparentWhite(const Canvas &src);
  void CopyTransparentBlack(const Canvas &src);

  void StretchTransparent(const Bitmap &src, Color key);
  void InvertStretchTransparent(const Bitmap &src, Color key);

  void Stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               SDL_Surface *src,
               int src_x, int src_y, unsigned src_width, unsigned src_height);

  void Stretch(SDL_Surface *src) {
    Stretch(0, 0, GetWidth(), GetHeight(),
            src, 0, 0, src->w, src->h);
  }

  void Stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height) {
    Stretch(dest_x, dest_y, dest_width, dest_height,
            src.surface,
            src_x, src_y, src_width, src_height);
  }

  void Stretch(const Canvas &src,
               int src_x, int src_y, unsigned src_width, unsigned src_height);

  void Stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src,
               int src_x, int src_y,
               unsigned src_width, unsigned src_height);
  void Stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src);

  void Stretch(const Bitmap &src) {
    Stretch(0, 0, size.cx, size.cy, src);
  }

  void StretchMono(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   const Bitmap &src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height,
                   Color fg_color, Color bg_color);

  void CopyNot(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               SDL_Surface *src, int src_x, int src_y);

  void CopyNot(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src, int src_x, int src_y);

  void CopyOr(int dest_x, int dest_y,
              unsigned dest_width, unsigned dest_height,
              SDL_Surface *src, int src_x, int src_y);

  void CopyOr(int dest_x, int dest_y,
              unsigned dest_width, unsigned dest_height,
              const Bitmap &src, int src_x, int src_y);

  void CopyOr(const Bitmap &src) {
    CopyOr(0, 0, GetWidth(), GetHeight(), src, 0, 0);
  }

  void CopyNotOr(int dest_x, int dest_y,
                 unsigned dest_width, unsigned dest_height,
                 SDL_Surface *src, int src_x, int src_y);

  void CopyNotOr(int dest_x, int dest_y,
                 unsigned dest_width, unsigned dest_height,
                 const Bitmap &src, int src_x, int src_y);

  void CopyAnd(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               SDL_Surface *src, int src_x, int src_y);

  void CopyAnd(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Canvas &src, int src_x, int src_y) {
    CopyAnd(dest_x, dest_y, dest_width, dest_height,
            src.surface, src_x, src_y);
  }

  void CopyAnd(const Canvas &src) {
    CopyAnd(0, 0, src.GetWidth(), src.GetHeight(), src, 0, 0);
  }

  void CopyAnd(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src, int src_x, int src_y);

  void CopyAnd(const Bitmap &src) {
    CopyAnd(0, 0, GetWidth(), GetHeight(), src, 0, 0);
  }

  void ScaleCopy(int dest_x, int dest_y,
                 const Bitmap &src,
                 int src_x, int src_y,
                 unsigned src_width, unsigned src_height);

  void AlphaBlend(int dest_x, int dest_y,
                  unsigned dest_width, unsigned dest_height,
                  SDL_Surface *src,
                  int src_x, int src_y,
                  unsigned src_width, unsigned src_height,
                  uint8_t alpha);

  void AlphaBlend(int dest_x, int dest_y,
                  unsigned dest_width, unsigned dest_height,
                  const Canvas &src,
                  int src_x, int src_y,
                  unsigned src_width, unsigned src_height,
                  uint8_t alpha);
};

#endif
