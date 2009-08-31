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

#ifdef PNA
#include "Asset.hpp" /* for needclipping */
#endif

#include <string.h>
#include <stdlib.h> /* for abs() */


void
Canvas::clipped_polygon(const POINT* lppt, unsigned cPoints, const RECT rc,
                        bool fill)
{
  ::ClipPolygon(*this, lppt, cPoints, rc, fill);
}

void
Canvas::clipped_polyline(const POINT* lppt, unsigned cPoints, const RECT rc)
{
#ifdef BUG_IN_CLIPPING
  ::ClipPolygon(dc, lppt, cPoints, rc, false);
  //VENTA2
#elif defined(PNA)
  // if (GlobalModelType == MODELTYPE_PNA_HP31X)
  if (needclipping)
    ::ClipPolygon(*this, lppt, cPoints, rc, false);
  else
    polyline(lppt, cPoints);
#else
  polyline(lppt, cPoints);
#endif
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
#ifdef NOLINETO
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
Canvas::text_size(const TCHAR *text) const
{
  SIZE size;
  ::GetTextExtentPoint(dc, text, _tcslen(text), &size);
  return size;
}

void
Canvas::text(int x, int y, const TCHAR *text)
{
  ::ExtTextOut(dc, x, y, 0, NULL, text, _tcslen(text), NULL);
}

void
Canvas::text_opaque(int x, int y, const RECT* lprc, const TCHAR *text)
{
  ::ExtTextOut(dc, x, y, ETO_OPAQUE, lprc, text, _tcslen(text), NULL);
}

void
Canvas::text_clipped(int x, int y, unsigned width, const TCHAR *text)
{
  const SIZE size = text_size(text);

  RECT rc;
  ::SetRect(&rc, x, y, x + min(width, size.cx), y + size.cy);
  ::ExtTextOut(dc, x, y, ETO_CLIPPED, &rc, text, _tcslen(text), NULL);
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

#if WINDOWSPC < 1
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
