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

#include "Screen/Canvas.hpp"
#include "Screen/Util.hpp"
#include "InfoBoxLayout.h"
#include "Compatibility/gdi.h"
#include "options.h" /* for IBLSCALE() */
#include "Asset.hpp" /* for needclipping */

#include <string.h>
#include <stdlib.h> /* for abs() */

#ifdef ENABLE_SDL

#include <SDL/SDL_rotozoom.h>
#include <SDL/SDL_imageFilter.h>


void
Canvas::reset()
{
  if (surface != NULL) {
    SDL_FreeSurface(surface);
    surface = NULL;
  }
}

void
Canvas::move_to(int x, int y)
{
  cursor.x = x;
  cursor.y = y;
}

void
Canvas::line_to(int x, int y)
{
  line(cursor.x, cursor.y, x, y);
  move_to(x, y);
}

void
Canvas::arc(int x, int y, unsigned radius, const RECT rc,
            double start, double end)
{
  // XXX
  ::pieColor(surface, x, y, radius, start - 90, end - 90,
             brush.get_color().gfx_color());
}

void
Canvas::segment(int x, int y, unsigned radius, const RECT rc,
                double start, double end, bool horizon)
{
  // XXX horizon

  if (!brush.is_hollow())
    ::filledPieColor(surface, x, y, radius, (int)start - 90, (int)end - 90,
                     brush.get_color().gfx_color());

  if (pen_over_brush())
    ::pieColor(surface, x, y, radius, (int)start - 90, (int)end - 90,
               pen.get_color().gfx_color());
}

void
Canvas::draw_button(RECT rc, bool down)
{
  Brush gray(Color(192, 192, 192));
  fill_rectangle(rc, gray);

  Pen bright(1, Color(240, 240, 240));
  Pen dark(1, Color(128, 128, 128));

  select(down ? dark : bright);
  two_lines(rc.left, rc.bottom - 2, rc.left, rc.top,
            rc.right - 2, rc.top);
  two_lines(rc.left + 1, rc.bottom - 3, rc.left + 1, rc.top + 1,
            rc.right - 3, rc.top + 1);

  select(down ? bright : dark);
  two_lines(rc.left + 1, rc.bottom - 1, rc.right - 1, rc.bottom - 1,
            rc.right - 1, rc.top + 1);
  two_lines(rc.left + 2, rc.bottom - 2, rc.right - 2, rc.bottom - 2,
            rc.right - 2, rc.top + 2);

  white_pen();
}

const SIZE
Canvas::text_size(const TCHAR *text, size_t length) const
{
  TCHAR *duplicated = _tcsdup(text);
  duplicated[length] = 0;

  const SIZE size = text_size(duplicated);
  free(duplicated);

  return size;
}

const SIZE
Canvas::text_size(LPCTSTR text) const
{
  SIZE size = { 0, 0 };

  if (font == NULL)
    return size;

  int ret, w, h;
#ifdef UNICODE
  ret = ::TTF_SizeUNICODE(font, (const Uint16 *)text, &w, &h);
#else
  ret = ::TTF_SizeText(font, text, &w, &h);
#endif
  if (ret == 0) {
    size.cx = w;
    size.cy = h;
  }

  return size;
}

void
Canvas::text(int x, int y, LPCTSTR text)
{
  SDL_Surface *s;

  if (font == NULL)
    return;

#ifdef UNICODE
  s = ::TTF_RenderUNICODE_Solid(font, (const Uint16 *)text, text_color);
#else
  s = ::TTF_RenderText_Solid(font, text, text_color);
#endif
  if (s == NULL)
    return;

  SDL_Rect dest = { x, y };
  // XXX non-opaque?
  ::SDL_BlitSurface(s, NULL, surface, &dest);
  ::SDL_FreeSurface(s);
}

void
Canvas::text_opaque(int x, int y, const RECT* lprc, LPCTSTR _text)
{
  // XXX
  text(x, y, _text);
}

void
Canvas::bottom_right_text(int x, int y, LPCTSTR _text)
{
  SIZE size = text_size(_text);
  text(x - size.cx, y - size.cy, _text);
}

void
Canvas::copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             const Canvas &src, int src_x, int src_y)
{
  SDL_Rect src_rect = { src_x, src_y, dest_width, dest_height };
  SDL_Rect dest_rect = { dest_x, dest_y };

  ::SDL_BlitSurface(src.surface, &src_rect, surface, &dest_rect);
}

void
Canvas::copy(const Canvas &src, int src_x, int src_y)
{
  copy(0, 0, src.surface->w, src.surface->h, src, src_x, src_y);
}

void
Canvas::copy(const Canvas &src)
{
  copy(src, 0, 0);
}

void
Canvas::copy_transparent_white(const Canvas &src, const RECT &rc)
{
  ::SDL_SetColorKey(src.surface, SDL_SRCCOLORKEY,
                    src.map(Color(0xff, 0xff, 0xff)));
  copy(src);
  ::SDL_SetColorKey(src.surface, 0, 0);
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  SDL_Surface *zoomed =
    ::zoomSurface(src.surface, (double)dest_width / (double)src_width,
                  (double)dest_height / (double)src_height,
                  SMOOTHING_OFF);

  if (zoomed == NULL)
    return;

  ::SDL_SetColorKey(zoomed, 0, 0);

  SDL_Rect src_rect = {
    (src_x * dest_width) / src_width,
    (src_y * dest_height) / src_height,
    dest_width, dest_height
  };
  SDL_Rect dest_rect = { dest_x, dest_y };

  ::SDL_BlitSurface(zoomed, &src_rect, surface, &dest_rect);
  ::SDL_FreeSurface(zoomed);
}

void
Canvas::stretch(const Canvas &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  // XXX
  stretch(0, 0, get_width(), get_height(),
          src, src_x, src_y, src_width, src_height);
}

static bool
clip_range(int &a, unsigned a_size, int &b, unsigned b_size, unsigned &size)
{
  if (a < 0) {
    b -= a;
    size += a;
    a = 0;
  }

  if (b < 0) {
    a -= b;
    size += b;
    b = 0;
  }

  if ((int)size <= 0)
    return false;

  if (a + size > a_size)
    size = a_size - a;

  if (b + size > b_size)
    size = b_size - b;

  return (int)size > 0;
}

static void
blit_or(SDL_Surface *dest, int dest_x, int dest_y,
        unsigned dest_width, unsigned dest_height,
        SDL_Surface *_src, int src_x, int src_y)
{
  int ret;

  /* obey the dest and src surface borders */

  if (!clip_range(dest_x, dest->w, src_x, _src->w, dest_width) ||
      !clip_range(dest_y, dest->h, src_y, _src->h, dest_height))
    return;

  ret = ::SDL_LockSurface(dest);
  if (ret != 0)
    return;

  /* convert src's pixel format */

  SDL_Surface *src = ::SDL_ConvertSurface(_src, dest->format, SDL_SWSURFACE);
  if (src == NULL) {
    ::SDL_UnlockSurface(dest);
    return;
  }

  ret = ::SDL_LockSurface(src);
  if (ret != 0) {
    ::SDL_FreeSurface(src);
    ::SDL_UnlockSurface(dest);
    return;
  }

  /* get pointers to the upper left dest/src pixel */

  unsigned char *dest_buffer = (unsigned char *)dest->pixels;
  dest_buffer += dest_y * dest->pitch +
    dest_x * dest->format->BytesPerPixel;

  unsigned char *src_buffer = (unsigned char *)src->pixels;
  src_buffer += src_y * src->pitch +
    src_x * src->format->BytesPerPixel;

  /* copy line by line */

  for (unsigned y = 0; y < dest_height; ++y) {
    ::SDL_imageFilterBitOr(src_buffer, dest_buffer, dest_buffer,
                           dest_width * dest->format->BytesPerPixel);
    src_buffer += src->pitch;
    dest_buffer += dest->pitch;
  }

  /* cleanup */

  ::SDL_UnlockSurface(src);
  ::SDL_FreeSurface(src);
  ::SDL_UnlockSurface(dest);
}

static void
blit_and(SDL_Surface *dest, int dest_x, int dest_y,
         unsigned dest_width, unsigned dest_height,
         SDL_Surface *_src, int src_x, int src_y)
{
  int ret;

  /* obey the dest and src surface borders */

  if (!clip_range(dest_x, dest->w, src_x, _src->w, dest_width) ||
      !clip_range(dest_y, dest->h, src_y, _src->h, dest_height))
    return;

  ret = ::SDL_LockSurface(dest);
  if (ret != 0)
    return;

  /* convert src's pixel format */

  SDL_Surface *src = ::SDL_ConvertSurface(_src, dest->format, SDL_SWSURFACE);
  if (src == NULL) {
    ::SDL_UnlockSurface(dest);
    return;
  }

  ret = ::SDL_LockSurface(src);
  if (ret != 0) {
    ::SDL_FreeSurface(src);
    ::SDL_UnlockSurface(dest);
    return;
  }

  /* get pointers to the upper left dest/src pixel */

  unsigned char *dest_buffer = (unsigned char *)dest->pixels;
  dest_buffer += dest_y * dest->pitch +
    dest_x * dest->format->BytesPerPixel;

  unsigned char *src_buffer = (unsigned char *)src->pixels;
  src_buffer += src_y * src->pitch +
    src_x * src->format->BytesPerPixel;

  /* copy line by line */

  for (unsigned y = 0; y < dest_height; ++y) {
    ::SDL_imageFilterBitAnd(src_buffer, dest_buffer, dest_buffer,
                            dest_width * dest->format->BytesPerPixel);
    src_buffer += src->pitch;
    dest_buffer += dest->pitch;
  }

  /* cleanup */

  ::SDL_UnlockSurface(src);
  ::SDL_FreeSurface(src);
  ::SDL_UnlockSurface(dest);
}

void
Canvas::copy_or(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src, int src_x, int src_y)
{
  ::blit_or(surface, dest_x, dest_y, dest_width, dest_height,
            src.surface, src_x, src_y);
}

void
Canvas::copy_and(int dest_x, int dest_y,
                 unsigned dest_width, unsigned dest_height,
                 const Canvas &src, int src_x, int src_y)
{
  ::blit_and(surface, dest_x, dest_y, dest_width, dest_height,
             src.surface, src_x, src_y);
}

void
Canvas::stretch_or(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   const Canvas &src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height)
{
  SDL_Surface *zoomed =
    ::zoomSurface(src.surface, (double)dest_width / (double)src_width,
                  (double)dest_height / (double)src_height,
                  SMOOTHING_OFF);

  if (zoomed == NULL)
    return;

  ::SDL_SetColorKey(zoomed, 0, 0);

  ::blit_or(surface, dest_x, dest_y, zoomed->w, zoomed->h,
            zoomed,
            (src_x * dest_width) / src_width,
            (src_y * dest_height) / src_height);
  ::SDL_FreeSurface(zoomed);
}

void
Canvas::stretch_and(int dest_x, int dest_y,
                    unsigned dest_width, unsigned dest_height,
                    const Canvas &src,
                    int src_x, int src_y,
                    unsigned src_width, unsigned src_height)
{
  SDL_Surface *zoomed =
    ::zoomSurface(src.surface, (double)dest_width / (double)src_width,
                  (double)dest_height / (double)src_height,
                  SMOOTHING_OFF);

  if (zoomed == NULL)
    return;

  ::SDL_SetColorKey(zoomed, 0, 0);

  ::blit_and(surface, dest_x, dest_y, zoomed->w, zoomed->h,
             zoomed,
             (src_x * dest_width) / src_width,
             (src_y * dest_height) / src_height);
  ::SDL_FreeSurface(zoomed);
}

#else /* !ENABLE_SDL */

// TODO: ClipPolygon is not thread safe (uses a static array)!
// We need to make it so.

void
Canvas::clipped_polygon(const POINT* lppt, unsigned cPoints, const RECT rc,
                        bool fill)
{
  ::ClipPolygon(*this, lppt, cPoints, rc, fill);
}

void
Canvas::clipped_polyline(const POINT* lppt, unsigned cPoints, const RECT rc)
{
  if (need_clipping())
    ::ClipPolygon(*this, lppt, cPoints, rc, false);
  else
    polyline(lppt, cPoints);
}

void
Canvas::clipped_line(const POINT a, const POINT b, const RECT rc)
{
  POINT p[2] = {{a.x, a.y}, {b.x, b.y}};
  clipped_polyline(p, 2, rc);
}

void
Canvas::clipped_dashed_line(int width, const POINT a, const POINT b,
                            const Color color, const RECT rc)
{
  int i;
  POINT pt[2];

  //Create a dot pen
  Pen pen(Pen::DASH, 1, color);
  select(pen);

  pt[0].x = a.x;
  pt[0].y = a.y;
  pt[1].x = b.x;
  pt[1].y = b.y;

  //increment on smallest variance
  if (abs(a.x - b.x) < abs(a.y - b.y)) {
    pt[0].x -= width / 2;
    pt[1].x -= width / 2;
    for (i = 0; i < width; i++){
      pt[0].x += 1;
      pt[1].x += 1;
      clipped_polyline(pt, 2, rc);
    }
  } else {
    pt[0].y -= width / 2;
    pt[1].y -= width / 2;
    for (i = 0; i < width; i++){
      pt[0].y += 1;
      pt[1].y += 1;
      clipped_polyline(pt, 2, rc);
    }
  }
}

void
Canvas::line(int ax, int ay, int bx, int by)
{
#ifndef NOLINETO
  ::MoveToEx(dc, ax, ay, NULL);
  ::LineTo(dc, bx, by);
#else
  POINT p[2] = {{ax, ay}, {bx, by}};
  polyline(p, 2);
#endif
}

void
Canvas::two_lines(int ax, int ay, int bx, int by, int cx, int cy)
{
#ifndef NOLINETO
  ::MoveToEx(dc, ax, ay, NULL);
  ::LineTo(dc, bx, by);
  ::LineTo(dc, cx, cy);
#else
  POINT p[2];

  p[0].x = ax;
  p[0].y = ay;
  p[1].x = bx;
  p[1].y = by;
  polyline(p, 2);

  p[0].x = cx;
  p[0].y = cy;
  polyline(p, 2);
#endif
}

#ifndef NOLINETO

void
Canvas::move_to(int x, int y)
{
  ::MoveToEx(dc, x, y, NULL);
}

void
Canvas::line_to(int x, int y)
{
  ::LineTo(dc, x, y);
}

#endif /* !NOLINETO */

void
Canvas::circle(int x, int y, unsigned radius,
               const RECT rc, bool clip, bool fill)
{
  ::Circle(*this, x, y, radius, rc, clip, fill);
}

void
Canvas::arc(int x, int y, unsigned radius, const RECT rc,
            double start, double end)
{
  ::DrawArc(*this, x, y, radius, rc, start, end);
}

void
Canvas::segment(int x, int y, unsigned radius, const RECT rc,
                double start, double end, bool horizon)
{
  ::Segment(*this, x, y, radius, rc, start, end, horizon);
}

const SIZE
Canvas::text_size(const TCHAR *text, size_t length) const
{
  SIZE size;
  ::GetTextExtentPoint(dc, text, length, &size);
  return size;
}

const SIZE
Canvas::text_size(const TCHAR *text) const
{
  return text_size(text, _tcslen(text));
}

void
Canvas::text(int x, int y, const TCHAR *text)
{
  ::ExtTextOut(dc, x, y, 0, NULL, text, _tcslen(text), NULL);
}

void
Canvas::text_opaque(int x, int y, const TCHAR *text, size_t length)
{
  ::ExtTextOut(dc, x, y, ETO_OPAQUE, NULL, text, length, NULL);
}

void
Canvas::text_opaque(int x, int y, const RECT* lprc, const TCHAR *text)
{
  ::ExtTextOut(dc, x, y, ETO_OPAQUE, lprc, text, _tcslen(text), NULL);
}

void
Canvas::text_clipped(int x, int y, const RECT &rc, const TCHAR *text)
{
  ::ExtTextOut(dc, x, y, ETO_CLIPPED, &rc, text, _tcslen(text), NULL);
}

void
Canvas::text_clipped(int x, int y, unsigned width, const TCHAR *text)
{
  const SIZE size = text_size(text);

  RECT rc;
  ::SetRect(&rc, x, y, x + min(width, (unsigned)size.cx), y + size.cy);
  text_clipped(x, y, rc, text);
}

void
Canvas::bottom_right_text(int x, int y, const TCHAR *text)
{
  size_t length = _tcslen(text);
  SIZE size;

  // XXX use SetTextAlign() instead?
  ::GetTextExtentPoint(dc, text, length, &size);
  ::ExtTextOut(dc, x - size.cx, y - size.cy,
               0, NULL, text, length, NULL);
}

void
Canvas::copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             const Canvas &src, int src_x, int src_y)
{
  ::BitBlt(dc, dest_x, dest_y, dest_width, dest_height,
           src.dc, src_x, src_y, SRCCOPY);
}

void
Canvas::copy(const Canvas &src, int src_x, int src_y)
{
  copy(0, 0, width, height, src, src_x, src_y);
}

void
Canvas::copy(const Canvas &src)
{
  copy(src, 0, 0);
}

void
Canvas::copy_transparent_white(const Canvas &src, const RECT &rc)
{
  static COLORREF whitecolor = RGB(0xff,0xff,0xff);

#if !defined(WINDOWSPC)
  ::TransparentImage(dc,
                     rc.left, rc.top,
                     rc.right - rc.left, rc.bottom - rc.top,
                     src.dc,
                     rc.left, rc.top,
                     rc.right - rc.left, rc.bottom - rc.top,
                     whitecolor);
#else
  ::TransparentBlt(dc,
                   rc.left, rc.top,
                   rc.right - rc.left, rc.bottom - rc.top,
                   src.dc,
                   rc.left, rc.top,
                   rc.right - rc.left, rc.bottom - rc.top,
                   whitecolor);
#endif
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  ::StretchBlt(dc, dest_x, dest_y, dest_width, dest_height,
               src.dc, src_x, src_y, src_width, src_height,
               SRCCOPY);
}

void
Canvas::stretch(const Canvas &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  stretch(0, 0, width, height, src, src_x, src_y, src_width, src_height);
}

void
Canvas::copy_or(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src, int src_x, int src_y)
{
  ::BitBlt(dc, dest_x, dest_y, dest_width, dest_height,
           src.dc, src_x, src_y, SRCPAINT);
}

void
Canvas::copy_and(int dest_x, int dest_y,
                 unsigned dest_width, unsigned dest_height,
                 const Canvas &src, int src_x, int src_y)
{
  ::BitBlt(dc, dest_x, dest_y, dest_width, dest_height,
           src.dc, src_x, src_y, SRCAND);
}

void
Canvas::stretch_or(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   const Canvas &src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height)
{
  ::StretchBlt(dc, dest_x, dest_y, dest_width, dest_height,
               src.dc, src_x, src_y, src_width, src_height,
               SRCPAINT);
}

void
Canvas::stretch_and(int dest_x, int dest_y,
                    unsigned dest_width, unsigned dest_height,
                    const Canvas &src,
                    int src_x, int src_y,
                    unsigned src_width, unsigned src_height)
{
  ::StretchBlt(dc, dest_x, dest_y, dest_width, dest_height,
               src.dc, src_x, src_y, src_width, src_height,
               SRCAND);
}

#endif /* !ENABLE_SDL */

void
Canvas::scale_copy(int dest_x, int dest_y,
                   const Canvas &src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height)
{
  if (InfoBoxLayout::scale > 1)
    stretch(dest_x, dest_y, IBLSCALE(src_width), IBLSCALE(src_height),
            src, src_x, src_y, src_width, src_height);
  else
    copy(dest_x, dest_y, src_width, src_height,
            src, src_x, src_y);
}

void
Canvas::scale_or(int dest_x, int dest_y,
                 const Canvas &src,
                 int src_x, int src_y,
                 unsigned src_width, unsigned src_height)
{
  if (InfoBoxLayout::scale > 1)
    stretch_or(dest_x, dest_y, IBLSCALE(src_width), IBLSCALE(src_height),
               src, src_x, src_y, src_width, src_height);
  else
    copy_or(dest_x, dest_y, src_width, src_height,
            src, src_x, src_y);
}

void
Canvas::scale_and(int dest_x, int dest_y,
                  const Canvas &src,
                  int src_x, int src_y,
                  unsigned src_width, unsigned src_height)
{
  if (InfoBoxLayout::scale > 1)
    stretch_and(dest_x, dest_y, IBLSCALE(src_width), IBLSCALE(src_height),
                src, src_x, src_y, src_width, src_height);
  else
    copy_and(dest_x, dest_y, src_width, src_height,
             src, src_x, src_y);
}
