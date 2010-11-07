/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Screen/Canvas.hpp"

#include <assert.h>
#include <string.h>

#ifndef ENABLE_OPENGL
#include <SDL_rotozoom.h>
#include <SDL_imageFilter.h>
#endif

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

#ifndef ENABLE_OPENGL

void
Canvas::polyline(const POINT *lppt, unsigned cPoints)
{
  for (unsigned i = 1; i < cPoints; ++i)
    line(lppt[i - 1].x, lppt[i - 1].y, lppt[i].x, lppt[i].y);
}

void
Canvas::polygon(const POINT* lppt, unsigned cPoints)
{
  if (brush.is_hollow() && !pen.defined())
    return;

  Sint16 vx[cPoints], vy[cPoints];

  for (unsigned i = 0; i < cPoints; ++i) {
    vx[i] = x_offset + lppt[i].x;
    vy[i] = y_offset + lppt[i].y;
  }

  if (!brush.is_hollow())
    ::filledPolygonColor(surface, vx, vy, cPoints,
                         brush.get_color().gfx_color());

  if (pen_over_brush())
    ::polygonColor(surface, vx, vy, cPoints, pen.get_color().gfx_color());
}

void
Canvas::segment(int x, int y, unsigned radius,
                Angle start, Angle end, bool horizon)
{
  // XXX horizon

  x += x_offset;
  y += y_offset;

  if (!brush.is_hollow())
    ::filledPieColor(surface, x, y, radius, 
                     (int)start.value_degrees() - 90,
                     (int)end.value_degrees() - 90,
                     brush.get_color().gfx_color());

  if (pen_over_brush())
    ::pieColor(surface, x, y, radius, 
               (int)start.value_degrees() - 90,
               (int)end.value_degrees() - 90,
               pen.get_color().gfx_color());
}

#endif /* !OPENGL */

void
Canvas::draw_button(RECT rc, bool down)
{
  const Pen old_pen = pen;

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

  pen = old_pen;
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
Canvas::text_size(const TCHAR *text) const
{
  SIZE size = { 0, 0 };

  if (font == NULL)
    return size;

  int ret, w, h;
  ret = ::TTF_SizeUTF8(font, text, &w, &h);
  if (ret == 0) {
    size.cx = w;
    size.cy = h;
  }

  return size;
}

#ifndef ENABLE_OPENGL

void
Canvas::text(int x, int y, const TCHAR *text)
{
  SDL_Surface *s;

  if (font == NULL)
    return;

  s = ::TTF_RenderUTF8_Solid(font, text, Color::BLACK);
  if (s == NULL)
    return;

  if (s->format->palette != NULL && s->format->palette->ncolors >= 2) {
    s->format->palette->colors[1] = text_color;

    if (background_mode == OPAQUE) {
      s->flags &= ~SDL_SRCCOLORKEY;
      s->format->palette->colors[0] = background_color;
    }
  }

  copy(x, y, s);
  ::SDL_FreeSurface(s);
}

#endif /* !OPENGL */

void
Canvas::text(int x, int y, const TCHAR *_text, size_t length)
{
  TCHAR copy[length + 1];
  _tcsncpy(copy, _text, length);
  copy[length] = _T('\0');
  text(x, y, copy);
}

void
Canvas::text_opaque(int x, int y, const RECT &rc, const TCHAR *_text)
{
  fill_rectangle(rc, background_color);
  text(x, y, _text);
}

#ifndef ENABLE_OPENGL

static bool
clip(int &position, unsigned &length, unsigned max)
{
  if (position < 0) {
    if (length <= (unsigned)-position)
      return false;

    length -= -position;
    position = 0;
  }

  if ((unsigned)position >= max)
    return false;

  if (position + length >= max)
    length = max - position;

  return true;
}

void
Canvas::copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             SDL_Surface *src_surface, int src_x, int src_y)
{
  assert(src_surface != NULL);

  if (!clip(dest_x, dest_width, width) ||
      !clip(dest_y, dest_height, height))
    return;

  SDL_Rect src_rect = { src_x, src_y, dest_width, dest_height };
  SDL_Rect dest_rect = { x_offset + dest_x, y_offset + dest_y };

  ::SDL_BlitSurface(src_surface, &src_rect, surface, &dest_rect);
}

#endif /* !OPENGL */

void
Canvas::copy(const Canvas &src, int src_x, int src_y)
{
  copy(0, 0, src.get_width(), src.get_height(), src, src_x, src_y);
}

void
Canvas::copy(const Canvas &src)
{
  copy(src, 0, 0);
}

#ifndef ENABLE_OPENGL

void
Canvas::copy_transparent_white(const Canvas &src)
{
  assert(src.surface != NULL);

  ::SDL_SetColorKey(src.surface, SDL_SRCCOLORKEY, src.map(Color::WHITE));
  copy(src);
  ::SDL_SetColorKey(src.surface, 0, 0);
}

void
Canvas::copy_transparent_black(const Canvas &src)
{
  assert(src.surface != NULL);

  ::SDL_SetColorKey(src.surface, SDL_SRCCOLORKEY, src.map(Color::BLACK));
  copy(src);
  ::SDL_SetColorKey(src.surface, 0, 0);
}

void
Canvas::stretch_transparent(const Canvas &src, Color key)
{
  assert(src.surface != NULL);

  ::SDL_SetColorKey(src.surface, SDL_SRCCOLORKEY, src.map(key));
  stretch(src);
  ::SDL_SetColorKey(src.surface, 0, 0);
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Canvas &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  assert(src.surface != NULL);

  SDL_Surface *zoomed =
    ::zoomSurface(src.surface, (double)dest_width / (double)src_width,
                  (double)dest_height / (double)src_height,
                  SMOOTHING_OFF);

  if (zoomed == NULL)
    return;

  ::SDL_SetColorKey(zoomed, 0, 0);

  copy(dest_x, dest_y, dest_width, dest_height,
       zoomed, (src_x * dest_width) / src_width,
       (src_y * dest_height) / src_height);
  ::SDL_FreeSurface(zoomed);
}

#endif /* !OPENGL */

void
Canvas::stretch(const Canvas &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  // XXX
  stretch(0, 0, get_width(), get_height(),
          src, src_x, src_y, src_width, src_height);
}

#ifndef ENABLE_OPENGL

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

  if ((int)size <= 0)
    return false;

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
  assert(src.surface != NULL);

  dest_x += x_offset;
  dest_y += y_offset;

  ::blit_or(surface, dest_x, dest_y, dest_width, dest_height,
            src.surface, src_x, src_y);
}

void
Canvas::copy_and(int dest_x, int dest_y,
                 unsigned dest_width, unsigned dest_height,
                 const Canvas &src, int src_x, int src_y)
{
  assert(src.surface != NULL);

  dest_x += x_offset;
  dest_y += y_offset;

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
  assert(src.surface != NULL);

  dest_x += x_offset;
  dest_y += y_offset;

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
  assert(src.surface != NULL);

  dest_x += x_offset;
  dest_y += y_offset;

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

#endif /* !OPENGL */
