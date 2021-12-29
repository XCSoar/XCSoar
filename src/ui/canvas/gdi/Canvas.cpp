/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Util.hpp"
#include "Asset.hpp" /* for needclipping */
#include "AlphaBlend.hpp"
#include "Math/Angle.hpp"

#include <algorithm>

void
Canvas::DrawLine(int ax, int ay, int bx, int by)
{
  assert(IsDefined());

#ifndef NOLINETO
  ::MoveToEx(dc, ax, ay, nullptr);
  ::LineTo(dc, bx, by);
#else
  BulkPixelPoint p[2] = {{ax, ay}, {bx, by}};
  DrawPolyline(p, 2);
#endif
}

void
Canvas::DrawTwoLines(int ax, int ay, int bx, int by, int cx, int cy)
{
  assert(IsDefined());

#ifndef NOLINETO
  ::MoveToEx(dc, ax, ay, nullptr);
  ::LineTo(dc, bx, by);
  ::LineTo(dc, cx, cy);
#else
  BulkPixelPoint p[2];

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
Canvas::DrawSegment(PixelPoint center, unsigned radius,
                    Angle start, Angle end, bool horizon)
{
  assert(IsDefined());

  ::Segment(*this, center, radius, start, end, horizon);
}

void
Canvas::DrawAnnulus(PixelPoint center,
                    unsigned small_radius, unsigned big_radius,
                    Angle start, Angle end)
{
  assert(IsDefined());

  ::Annulus(*this, center, big_radius, start, end, small_radius);
}

void
Canvas::DrawKeyhole(PixelPoint center,
                    unsigned small_radius, unsigned big_radius,
                    Angle start, Angle end)
{
  assert(IsDefined());

  ::KeyHole(*this, center, big_radius, start, end, small_radius);
}

void
Canvas::DrawArc(PixelPoint center, unsigned radius,
                Angle start, Angle end)
{
  assert(IsDefined());

  ::Arc(*this, center, radius, start, end);
}

const PixelSize
Canvas::CalcTextSize(BasicStringView<TCHAR> text) const noexcept
{
  assert(IsDefined());

  SIZE size;
  ::GetTextExtentPoint(dc, text.data, text.size, &size);
  return PixelSize(size.cx, size.cy);
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
Canvas::DrawText(PixelPoint p, BasicStringView<TCHAR> text) noexcept
{
  assert(IsDefined());

  ::ExtTextOut(dc, p.x, p.y, 0, nullptr, text.data, text.size, nullptr);
}

void
Canvas::DrawOpaqueText(PixelPoint p, const PixelRect &_rc,
                       BasicStringView<TCHAR> text) noexcept
{
  assert(IsDefined());

  RECT rc = _rc;
  ::ExtTextOut(dc, p.x, p.y, ETO_OPAQUE, &rc, text.data, text.size, nullptr);
}

void
Canvas::DrawClippedText(PixelPoint p, const PixelRect &_rc,
                        BasicStringView<TCHAR> text) noexcept
{
  assert(IsDefined());

  RECT rc = _rc;
  ::ExtTextOut(dc, p.x, p.y, ETO_CLIPPED, &rc, text.data, text.size, nullptr);
}

void
Canvas::DrawClippedText(PixelPoint p, unsigned width,
                        BasicStringView<TCHAR> text) noexcept
{
  const PixelSize size = CalcTextSize(text);

  RECT rc;
  ::SetRect(&rc, p.x, p.y,
            p.x + std::min(width, size.width), p.y + size.height);
  ::ExtTextOut(dc, p.x, p.y, ETO_CLIPPED, &rc, text.data, text.size, nullptr);
}

void
Canvas::Copy(PixelPoint dest_position, PixelSize dest_size,
             HBITMAP src, PixelPoint src_position,
             DWORD dwRop)
{
  assert(IsDefined());
  assert(src != nullptr);

  HDC virtual_dc = GetCompatibleDC();
  HBITMAP old = (HBITMAP)::SelectObject(virtual_dc, src);
  Copy(dest_position, dest_size, virtual_dc, src_position, dwRop);
  ::SelectObject(virtual_dc, old);
}

void
Canvas::Copy(PixelPoint dest_position, PixelSize dest_size,
             const Bitmap &src, PixelPoint src_position,
             DWORD dwRop)
{
  Copy(dest_position, dest_size, src.GetNative(), src_position, dwRop);
}

void
Canvas::Copy(const Canvas &src, PixelPoint src_position) noexcept
{
  Copy({0, 0}, GetSize(), src, src_position);
}

void
Canvas::Copy(const Canvas &src)
{
  Copy(src, {0, 0});
}

void
Canvas::Copy(const Bitmap &src)
{
  Copy({0, 0}, src.GetSize(), src, {0, 0});
}

void
Canvas::CopyTransparentWhite(PixelPoint dest_position, PixelSize dest_size,
                             const Canvas &src,
                             PixelPoint src_position) noexcept
{
  assert(IsDefined());
  assert(src.IsDefined());

  ::TransparentBlt(dc, dest_position.x, dest_position.y,
                   dest_size.width, dest_size.height,
                   src.dc,
                   src_position.x, src_position.y,
                   dest_size.width, dest_size.height,
                   COLOR_WHITE);
}

void
Canvas::StretchNot(const Bitmap &src)
{
  assert(IsDefined());
  assert(src.IsDefined());

  Stretch({0, 0}, GetSize(),
          src.GetNative(), {0, 0}, src.GetSize(),
          NOTSRCCOPY);
}

void
Canvas::Stretch(PixelPoint dest_position, PixelSize dest_size,
                HBITMAP src,
                PixelPoint src_position, PixelSize src_size,
                DWORD dwRop)
{
  assert(IsDefined());
  assert(src != nullptr);

  HDC virtual_dc = GetCompatibleDC();
  HBITMAP old = (HBITMAP)::SelectObject(virtual_dc, src);
  Stretch(dest_position, dest_size,
          virtual_dc, src_position, src_size,
          dwRop);
  ::SelectObject(virtual_dc, old);
}

void
Canvas::Stretch(PixelPoint dest_position, PixelSize dest_size,
                const Bitmap &src,
                PixelPoint src_position, PixelSize src_size,
                DWORD dwRop)
{
  assert(IsDefined());
  assert(src.IsDefined());

  Stretch(dest_position, dest_size,
          src.GetNative(), src_position, src_size,
          dwRop);
}

void
Canvas::Stretch(const Canvas &src,
                PixelPoint src_position, PixelSize src_size) noexcept
{
  Stretch({0, 0}, GetSize(),
          src, src_position, src_size);
}

void
Canvas::Stretch(PixelPoint dest_position, PixelSize dest_size,
                const Bitmap &src)
{
  assert(src.IsDefined());

  Stretch(dest_position, dest_size,
          src, {0, 0}, src.GetSize());
}

void
Canvas::Stretch(const Bitmap &src)
{
  assert(src.IsDefined());

  Stretch(src, {0, 0}, src.GetSize());
}

void
Canvas::StretchMono(PixelPoint dest_position, PixelSize dest_size,
                    const Bitmap &src,
                    PixelPoint src_position, PixelSize src_size,
                    Color fg_color, Color bg_color)
{
  assert(IsDefined());
  assert(src.IsDefined());

  if (bg_color == COLOR_BLACK && src_size != dest_size) {
    /* workaround for a WINE bug: stretching a mono bitmap ignores the
       text color; this kludge makes the text color white */
    SetTextColor(COLOR_BLACK);
    SetBackgroundColor(COLOR_WHITE);
    Stretch(dest_position, dest_size,
            src, src_position, src_size,
            MERGEPAINT);
    return;
  }

  /* on GDI, monochrome bitmaps are special: they are painted with the
     destination HDC's current colors */
  SetTextColor(fg_color);
  SetBackgroundTransparent();

  Stretch(dest_position, dest_size,
          src, src_position, src_size);
}

#ifdef HAVE_ALPHA_BLEND

void
Canvas::AlphaBlend(PixelPoint dest_position, PixelSize dest_size,
                   HDC src,
                   PixelPoint src_position, PixelSize src_size,
                   uint8_t alpha)
{
  BLENDFUNCTION fn;
  fn.BlendOp = AC_SRC_OVER;
  fn.BlendFlags = 0;
  fn.SourceConstantAlpha = alpha;
  fn.AlphaFormat = 0;

  ::AlphaBlend(dc, dest_position.x, dest_position.y,
               dest_size.width, dest_size.height,
               src, src_position.x, src_position.y,
               src_size.width, src_size.height,
               fn);
}

#endif
