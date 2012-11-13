/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
Canvas::DrawLine(PixelScalar ax, PixelScalar ay, PixelScalar bx, PixelScalar by)
{
  assert(IsDefined());

#ifndef NOLINETO
  ::MoveToEx(dc, ax, ay, NULL);
  ::LineTo(dc, bx, by);
#else
  RasterPoint p[2] = {{ax, ay}, {bx, by}};
  DrawPolyline(p, 2);
#endif
}

void
Canvas::DrawTwoLines(PixelScalar ax, PixelScalar ay,
                  PixelScalar bx, PixelScalar by,
                  PixelScalar cx, PixelScalar cy)
{
  assert(IsDefined());

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
  DrawPolyline(p, 2);

  p[0].x = cx;
  p[0].y = cy;
  DrawPolyline(p, 2);
#endif
}

void
Canvas::DrawSegment(PixelScalar x, PixelScalar y, UPixelScalar radius,
                Angle start, Angle end, bool horizon)
{
  assert(IsDefined());

  ::Segment(*this, x, y, radius, start, end, horizon);
}

void
Canvas::DrawAnnulus(PixelScalar x, PixelScalar y,
                UPixelScalar small_radius, UPixelScalar big_radius,
                Angle start, Angle end)
{
  assert(IsDefined());

  ::Annulus(*this, x, y, big_radius, start, end, small_radius);
}

void
Canvas::DrawKeyhole(PixelScalar x, PixelScalar y,
                UPixelScalar small_radius, UPixelScalar big_radius,
                Angle start, Angle end)
{
  assert(IsDefined());

  ::KeyHole(*this, x, y, big_radius, start, end, small_radius);
}

const PixelSize
Canvas::CalcTextSize(const TCHAR *text, size_t length) const
{
  assert(IsDefined());

  PixelSize size;
  ::GetTextExtentPoint(dc, text, length, &size);
  return size;
}

const PixelSize
Canvas::CalcTextSize(const TCHAR *text) const
{
  return CalcTextSize(text, _tcslen(text));
}

UPixelScalar
Canvas::GetFontHeight() const
{
  assert(IsDefined());

  TEXTMETRIC tm;
  GetTextMetrics(dc, &tm);
  return tm.tmHeight;
}

void
Canvas::DrawText(PixelScalar x, PixelScalar y, const TCHAR *text)
{
  assert(IsDefined());

  ::ExtTextOut(dc, x, y, 0, NULL, text, _tcslen(text), NULL);
}

void
Canvas::DrawText(PixelScalar x, PixelScalar y,
                 const TCHAR *text, size_t length)
{
  assert(IsDefined());

  ::ExtTextOut(dc, x, y, 0, NULL, text, length, NULL);
}

void
Canvas::DrawOpaqueText(PixelScalar x, PixelScalar y, const PixelRect &rc,
                       const TCHAR *text)
{
  assert(IsDefined());

  ::ExtTextOut(dc, x, y, ETO_OPAQUE, &rc, text, _tcslen(text), NULL);
}

void
Canvas::DrawClippedText(PixelScalar x, PixelScalar y, const PixelRect &rc,
                        const TCHAR *text)
{
  assert(IsDefined());

  ::ExtTextOut(dc, x, y, ETO_CLIPPED, &rc, text, _tcslen(text), NULL);
}

void
Canvas::DrawClippedText(PixelScalar x, PixelScalar y, UPixelScalar width,
                        const TCHAR *text)
{
  const PixelSize size = CalcTextSize(text);

  PixelRect rc;
  ::SetRect(&rc, x, y, x + min(width, (UPixelScalar)size.cx), y + size.cy);
  DrawClippedText(x, y, rc, text);
}

void
Canvas::Copy(PixelScalar dest_x, PixelScalar dest_y,
             UPixelScalar dest_width, UPixelScalar dest_height,
             HBITMAP src, PixelScalar src_x, PixelScalar src_y,
             DWORD dwRop)
{
  assert(IsDefined());
  assert(src != NULL);

  HDC virtual_dc = GetCompatibleDC();
  HBITMAP old = (HBITMAP)::SelectObject(virtual_dc, src);
  Copy(dest_x, dest_y, dest_width, dest_height,
       virtual_dc, src_x, src_y,
       dwRop);
  ::SelectObject(virtual_dc, old);
}

void
Canvas::Copy(PixelScalar dest_x, PixelScalar dest_y,
             UPixelScalar dest_width, UPixelScalar dest_height,
             const Bitmap &src, PixelScalar src_x, PixelScalar src_y,
             DWORD dwRop)
{
  Copy(dest_x, dest_y, dest_width, dest_height,
       src.GetNative(), src_x, src_y,
       dwRop);
}

void
Canvas::Copy(const Canvas &src, PixelScalar src_x, PixelScalar src_y)
{
  Copy(0, 0, width, height, src, src_x, src_y);
}

void
Canvas::Copy(const Canvas &src)
{
  Copy(src, 0, 0);
}

void
Canvas::Copy(const Bitmap &src)
{
  const PixelSize size = src.GetSize();
  Copy(0, 0, size.cx, size.cy, src, 0, 0);
}

void
Canvas::CopyTransparentBlack(const Canvas &src)
{
  assert(IsDefined());
  assert(src.IsDefined());

#ifdef _WIN32_WCE
  ::TransparentImage(dc, 0, 0, GetWidth(), GetHeight(),
                     src.dc, 0, 0, GetWidth(), GetHeight(),
                     COLOR_BLACK);
#else
  ::TransparentBlt(dc, 0, 0, GetWidth(), GetHeight(),
                   src.dc, 0, 0, GetWidth(), GetHeight(),
                   COLOR_BLACK);
#endif
}

void
Canvas::CopyTransparentWhite(const Canvas &src)
{
  assert(IsDefined());
  assert(src.IsDefined());

#ifdef _WIN32_WCE
  ::TransparentImage(dc, 0, 0, GetWidth(), GetHeight(),
                     src.dc, 0, 0, GetWidth(), GetHeight(),
                     COLOR_WHITE);
#else
  ::TransparentBlt(dc, 0, 0, GetWidth(), GetHeight(),
                   src.dc, 0, 0, GetWidth(), GetHeight(),
                   COLOR_WHITE);
#endif
}

void
Canvas::StretchTransparent(const Bitmap &src, Color key)
{
  assert(IsDefined());
  assert(src.IsDefined());

  HDC virtual_dc = GetCompatibleDC();
  HBITMAP old = (HBITMAP)::SelectObject(virtual_dc, src.GetNative());

  const PixelSize size = src.GetSize();
#ifdef _WIN32_WCE
  ::TransparentImage(dc, 0, 0, GetWidth(), GetHeight(),
                     virtual_dc, 0, 0, size.cx, size.cy,
                     key);
#else
  ::TransparentBlt(dc, 0, 0, GetWidth(), GetHeight(),
                   virtual_dc, 0, 0, size.cx, size.cy,
                   key);
#endif

  ::SelectObject(virtual_dc, old);
}

void
Canvas::InvertStretchTransparent(const Bitmap &src, Color key)
{
  assert(IsDefined());
  assert(src.IsDefined());

  HDC virtual_dc = GetCompatibleDC();
  HBITMAP old = (HBITMAP)::SelectObject(virtual_dc, src.GetNative());
  const PixelSize size = src.GetSize();

  BufferCanvas inverted(*this, size.cx, size.cy);
  ::BitBlt(inverted, 0, 0, size.cx, size.cy,
           virtual_dc, 0, 0, NOTSRCCOPY);
  ::SelectObject(virtual_dc, old);

#ifdef _WIN32_WCE
  ::TransparentImage(dc, 0, 0, GetWidth(), GetHeight(),
                     inverted, 0, 0, size.cx, size.cy,
                     key);
#else
  ::TransparentBlt(dc, 0, 0, GetWidth(), GetHeight(),
                   inverted, 0, 0, size.cx, size.cy,
                   key);
#endif
}

void
Canvas::Stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                HBITMAP src,
                PixelScalar src_x, PixelScalar src_y,
                UPixelScalar src_width, UPixelScalar src_height,
                DWORD dwRop)
{
  assert(IsDefined());
  assert(src != NULL);

  HDC virtual_dc = GetCompatibleDC();
  HBITMAP old = (HBITMAP)::SelectObject(virtual_dc, src);
  Stretch(dest_x, dest_y, dest_width, dest_height,
          virtual_dc, src_x, src_y, src_width, src_height,
          dwRop);
  ::SelectObject(virtual_dc, old);
}

void
Canvas::Stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src,
                PixelScalar src_x, PixelScalar src_y,
                UPixelScalar src_width, UPixelScalar src_height,
                DWORD dwRop)
{
  assert(IsDefined());
  assert(src.IsDefined());

  Stretch(dest_x, dest_y, dest_width, dest_height,
          src.GetNative(), src_x, src_y, src_width, src_height,
          dwRop);
}

void
Canvas::Stretch(const Canvas &src,
                PixelScalar src_x, PixelScalar src_y,
                UPixelScalar src_width, UPixelScalar src_height)
{
  Stretch(0, 0, width, height, src, src_x, src_y, src_width, src_height);
}

void
Canvas::Stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src)
{
  assert(src.IsDefined());

  const PixelSize size = src.GetSize();
  Stretch(dest_x, dest_y, dest_width, dest_height,
          src, 0, 0, size.cx, size.cy);
}

void
Canvas::Stretch(const Bitmap &src)
{
  assert(src.IsDefined());

  const PixelSize size = src.GetSize();
  Stretch(src, 0, 0, size.cx, size.cy);
}

void
Canvas::StretchMono(PixelScalar dest_x, PixelScalar dest_y,
                    UPixelScalar dest_width, UPixelScalar dest_height,
                    const Bitmap &src,
                    PixelScalar src_x, PixelScalar src_y,
                    UPixelScalar src_width, UPixelScalar src_height,
                    Color fg_color, Color bg_color)
{
  assert(IsDefined());
  assert(src.IsDefined());

#ifndef _WIN32_WCE
  if (bg_color == COLOR_BLACK && (src_width != dest_width ||
                                  src_height != dest_height)) {
    /* workaround for a WINE bug: stretching a mono bitmap ignores the
       text color; this kludge makes the text color white */
    SetTextColor(COLOR_BLACK);
    SetBackgroundColor(COLOR_WHITE);
    Stretch(dest_x, dest_y, dest_width, dest_height,
            src, src_x, src_y, src_width, src_height,
            MERGEPAINT);
    return;
  }
#endif

  /* on GDI, monochrome bitmaps are special: they are painted with the
     destination HDC's current colors */
  SetTextColor(fg_color);
  SetBackgroundTransparent();

  Stretch(dest_x, dest_y, dest_width, dest_height,
          src, src_x, src_y, src_width, src_height);
}
