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

#include "Screen/Canvas.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Util.hpp"
#include "Compatibility/gdi.h"
#include "Asset.hpp" /* for needclipping */

void
Canvas::line(int ax, int ay, int bx, int by)
{
  assert(defined());

#ifndef NOLINETO
  ::MoveToEx(dc, ax, ay, NULL);
  ::LineTo(dc, bx, by);
#else
  RasterPoint p[2] = {{ax, ay}, {bx, by}};
  polyline(p, 2);
#endif
}

void
Canvas::two_lines(int ax, int ay, int bx, int by, int cx, int cy)
{
  assert(defined());

#ifndef NOLINETO
  ::MoveToEx(dc, ax, ay, NULL);
  ::LineTo(dc, bx, by);
  ::LineTo(dc, cx, cy);
#else
  RasterPoint p[2];

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

void
Canvas::segment(int x, int y, unsigned radius,
                Angle start, Angle end, bool horizon)
{
  assert(defined());

  ::Segment(*this, x, y, radius, start, end, horizon);
}

void 
Canvas::annulus(int x, int y, unsigned small_radius, unsigned big_radius,
                Angle start, Angle end)
{
  assert(defined());

  ::Annulus(*this, x, y, big_radius, start, end, small_radius);
}

void
Canvas::keyhole(int x, int y, unsigned small_radius, unsigned big_radius,
                Angle start, Angle end)
{
  assert(defined());

  ::KeyHole(*this, x, y, big_radius, start, end, small_radius);
}

const PixelSize
Canvas::text_size(const TCHAR *text, size_t length) const
{
  assert(defined());

  PixelSize size;
  ::GetTextExtentPoint(dc, text, length, &size);
  return size;
}

const PixelSize
Canvas::text_size(const TCHAR *text) const
{
  return text_size(text, _tcslen(text));
}

unsigned
Canvas::text_height(const TCHAR *text) const
{
  assert(defined());

  TEXTMETRIC tm;
  GetTextMetrics(dc, &tm);
  return tm.tmHeight;
}

void
Canvas::text(int x, int y, const TCHAR *text)
{
  assert(defined());

  ::ExtTextOut(dc, x, y, 0, NULL, text, _tcslen(text), NULL);
}

void
Canvas::text(int x, int y, const TCHAR *text, size_t length)
{
  assert(defined());

  ::ExtTextOut(dc, x, y, 0, NULL, text, length, NULL);
}

void
Canvas::text_opaque(int x, int y, const PixelRect &rc, const TCHAR *text)
{
  assert(defined());

  ::ExtTextOut(dc, x, y, ETO_OPAQUE, &rc, text, _tcslen(text), NULL);
}

void
Canvas::text_clipped(int x, int y, const PixelRect &rc, const TCHAR *text)
{
  assert(defined());

  ::ExtTextOut(dc, x, y, ETO_CLIPPED, &rc, text, _tcslen(text), NULL);
}

void
Canvas::text_clipped(int x, int y, unsigned width, const TCHAR *text)
{
  const PixelSize size = text_size(text);

  PixelRect rc;
  ::SetRect(&rc, x, y, x + min(width, (unsigned)size.cx), y + size.cy);
  text_clipped(x, y, rc, text);
}

void
Canvas::copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             HBITMAP src, int src_x, int src_y,
             DWORD dwRop)
{
  assert(defined());
  assert(src != NULL);

  if (compatible_dc == NULL)
    compatible_dc = ::CreateCompatibleDC(dc);

  HBITMAP old = (HBITMAP)::SelectObject(compatible_dc, src);
  copy(dest_x, dest_y, dest_width, dest_height,
       compatible_dc, src_x, src_y,
       dwRop);
  ::SelectObject(compatible_dc, old);
}

void
Canvas::copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             const Bitmap &src, int src_x, int src_y,
             DWORD dwRop)
{
  copy(dest_x, dest_y, dest_width, dest_height,
       src.native(), src_x, src_y,
       dwRop);
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
Canvas::copy(const Bitmap &src)
{
  PixelSize size = src.get_size();
  copy(0, 0, size.cx, size.cy, src, 0, 0);
}

void
Canvas::copy_transparent_black(const Canvas &src)
{
  assert(defined());
  assert(src.defined());

#ifdef _WIN32_WCE
  ::TransparentImage(dc, 0, 0, get_width(), get_height(),
                     src.dc, 0, 0, get_width(), get_height(),
                     Color::BLACK);
#else
  ::TransparentBlt(dc, 0, 0, get_width(), get_height(),
                   src.dc, 0, 0, get_width(), get_height(),
                   Color::BLACK);
#endif
}

void
Canvas::copy_transparent_white(const Canvas &src)
{
  assert(defined());
  assert(src.defined());

#ifdef _WIN32_WCE
  ::TransparentImage(dc, 0, 0, get_width(), get_height(),
                     src.dc, 0, 0, get_width(), get_height(),
                     Color::WHITE);
#else
  ::TransparentBlt(dc, 0, 0, get_width(), get_height(),
                   src.dc, 0, 0, get_width(), get_height(),
                   Color::WHITE);
#endif
}

void
Canvas::stretch_transparent(const Bitmap &src, Color key)
{
  assert(defined());
  assert(src.defined());

  if (compatible_dc == NULL)
    compatible_dc = ::CreateCompatibleDC(dc);

  HBITMAP old = (HBITMAP)::SelectObject(compatible_dc, src.native());

  PixelSize size = src.get_size();
#ifdef _WIN32_WCE
  ::TransparentImage(dc, 0, 0, get_width(), get_height(),
                     compatible_dc, 0, 0, size.cx, size.cy,
                     key);
#else
  ::TransparentBlt(dc, 0, 0, get_width(), get_height(),
                   compatible_dc, 0, 0, size.cx, size.cy,
                   key);
#endif

  ::SelectObject(compatible_dc, old);
}

void
Canvas::invert_stretch_transparent(const Bitmap &src, Color key)
{
  assert(defined());
  assert(src.defined());

  if (compatible_dc == NULL)
    compatible_dc = ::CreateCompatibleDC(dc);

  HBITMAP old = (HBITMAP)::SelectObject(compatible_dc, src.native());
  const PixelSize size = src.get_size();

  BufferCanvas inverted(*this, size.cx, size.cy);
  ::BitBlt(inverted, 0, 0, size.cx, size.cy,
           compatible_dc, 0, 0, NOTSRCCOPY);
  ::SelectObject(compatible_dc, old);

#ifdef _WIN32_WCE
  ::TransparentImage(dc, 0, 0, get_width(), get_height(),
                     inverted, 0, 0, size.cx, size.cy,
                     key);
#else
  ::TransparentBlt(dc, 0, 0, get_width(), get_height(),
                   inverted, 0, 0, size.cx, size.cy,
                   key);
#endif
}

void
Canvas::stretch(const Canvas &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  stretch(0, 0, width, height, src, src_x, src_y, src_width, src_height);
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                HBITMAP src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  assert(defined());
  assert(src != NULL);

  if (compatible_dc == NULL)
    compatible_dc = ::CreateCompatibleDC(dc);

  HBITMAP old = (HBITMAP)::SelectObject(compatible_dc, src);
  stretch(dest_x, dest_y, dest_width, dest_height,
          compatible_dc, src_x, src_y, src_width, src_height);
  ::SelectObject(compatible_dc, old);
}

void
Canvas::stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  assert(defined());
  assert(src.defined());

  stretch(dest_x, dest_y, dest_width, dest_height,
          src.native(), src_x, src_y, src_width, src_height);
}

void
Canvas::stretch(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src)
{
  assert(src.defined());

  PixelSize size = src.get_size();
  stretch(dest_x, dest_y, dest_width, dest_height,
          src, 0, 0, size.cx, size.cy);
}

void
Canvas::stretch(const Bitmap &src)
{
  assert(src.defined());

  PixelSize size = src.get_size();
  stretch(src, 0, 0, size.cx, size.cy);
}
