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

#include "Screen/Canvas.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Util.hpp"
#include "Compatibility/gdi.h"
#include "Asset.hpp" /* for needclipping */
#include "AlphaBlend.hpp"

#include <algorithm>

void
Canvas::DrawLine(int ax, int ay, int bx, int by)
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
Canvas::DrawTwoLines(int ax, int ay, int bx, int by, int cx, int cy)
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
Canvas::DrawSegment(int x, int y, unsigned radius,
                    Angle start, Angle end, bool horizon)
{
  assert(IsDefined());

  ::Segment(*this, x, y, radius, start, end, horizon);
}

void
Canvas::DrawAnnulus(int x, int y,
                    unsigned small_radius, unsigned big_radius,
                    Angle start, Angle end)
{
  assert(IsDefined());

  ::Annulus(*this, x, y, big_radius, start, end, small_radius);
}

void
Canvas::DrawKeyhole(int x, int y,
                    unsigned small_radius, unsigned big_radius,
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

unsigned
Canvas::GetFontHeight() const
{
  assert(IsDefined());

  TEXTMETRIC tm;
  GetTextMetrics(dc, &tm);
  return tm.tmHeight;
}

void
Canvas::DrawText(int x, int y, const TCHAR *text)
{
  assert(IsDefined());

  ::ExtTextOut(dc, x, y, 0, NULL, text, _tcslen(text), NULL);
}

void
Canvas::DrawText(int x, int y,
                 const TCHAR *text, size_t length)
{
  assert(IsDefined());

  ::ExtTextOut(dc, x, y, 0, NULL, text, length, NULL);
}

void
Canvas::DrawOpaqueText(int x, int y, const PixelRect &rc,
                       const TCHAR *text)
{
  assert(IsDefined());

  ::ExtTextOut(dc, x, y, ETO_OPAQUE, &rc, text, _tcslen(text), NULL);
}

void
Canvas::DrawClippedText(int x, int y, const PixelRect &rc,
                        const TCHAR *text)
{
  assert(IsDefined());

  ::ExtTextOut(dc, x, y, ETO_CLIPPED, &rc, text, _tcslen(text), NULL);
}

void
Canvas::DrawClippedText(int x, int y, unsigned width,
                        const TCHAR *text)
{
  const PixelSize size = CalcTextSize(text);

  PixelRect rc;
  ::SetRect(&rc, x, y, x + std::min(width, unsigned(size.cx)), y + size.cy);
  DrawClippedText(x, y, rc, text);
}

void
Canvas::Copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             HBITMAP src, int src_x, int src_y,
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
Canvas::Copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             const Bitmap &src, int src_x, int src_y,
             DWORD dwRop)
{
  Copy(dest_x, dest_y, dest_width, dest_height,
       src.GetNative(), src_x, src_y,
       dwRop);
}

void
Canvas::Copy(const Canvas &src, int src_x, int src_y)
{
  Copy(0, 0, GetWidth(), GetHeight(), src, src_x, src_y);
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

  BufferCanvas inverted(*this, size);
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
Canvas::Stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                HBITMAP src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height,
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
Canvas::Stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height,
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
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  Stretch(0, 0, GetWidth(), GetHeight(),
          src, src_x, src_y, src_width, src_height);
}

void
Canvas::Stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
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
Canvas::StretchMono(int dest_x, int dest_y,
                    unsigned dest_width, unsigned dest_height,
                    const Bitmap &src,
                    int src_x, int src_y,
                    unsigned src_width, unsigned src_height,
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

#ifdef HAVE_ALPHA_BLEND

void
Canvas::AlphaBlend(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   HDC src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height,
                   uint8_t alpha)
{
  assert(AlphaBlendAvailable());

  BLENDFUNCTION fn;
  fn.BlendOp = AC_SRC_OVER;
  fn.BlendFlags = 0;
  fn.SourceConstantAlpha = alpha;
  fn.AlphaFormat = 0;

  ::AlphaBlendInvoke(dc, dest_x, dest_y, dest_width, dest_height,
                     src, src_x, src_y, src_width, src_height,
                     fn);
}

#endif
