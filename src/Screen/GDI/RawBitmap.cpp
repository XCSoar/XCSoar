/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "../RawBitmap.hpp"
#include "Canvas.hpp"

#include <assert.h>

/**
 * Returns minimum width that is greater then the given width and
 * that is acceptable as image width (not all numbers are acceptable)
 */
static inline unsigned
CorrectedWidth(unsigned nWidth)
{
  return ((nWidth + 3) / 4) * 4;
}

RawBitmap::RawBitmap(unsigned nWidth, unsigned nHeight)
  :width(nWidth), height(nHeight),
   corrected_width(CorrectedWidth(nWidth)),
   buffer(new RawColor[corrected_width * height])
{
  assert(nWidth > 0);
  assert(nHeight > 0);

  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = corrected_width;
  bi.bmiHeader.biHeight = height;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 24;
  bi.bmiHeader.biCompression = BI_RGB;
  bi.bmiHeader.biSizeImage = 0;
  bi.bmiHeader.biXPelsPerMeter = 3780;
  bi.bmiHeader.biYPelsPerMeter = 3780;
  bi.bmiHeader.biClrUsed = 0;
  bi.bmiHeader.biClrImportant = 0;

  VOID *pvBits;
  HDC hDC = ::GetDC(nullptr);
  bitmap = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
  ::ReleaseDC(nullptr, hDC);
  buffer = (RawColor *)pvBits;
}

RawBitmap::~RawBitmap()
{
  ::DeleteObject(bitmap);
}

void
RawBitmap::StretchTo(unsigned width, unsigned height,
                     Canvas &dest_canvas,
                     unsigned dest_width, unsigned dest_height,
                     bool transparent_white) const
{
  HDC source_dc = ::CreateCompatibleDC(dest_canvas);
  ::SelectObject(source_dc, bitmap);
  if (transparent_white)
    ::TransparentBlt(dest_canvas, 0, 0, dest_width, dest_height,
                     source_dc, 0, 0, width, height,
                     COLOR_WHITE);
  else
    ::StretchBlt(dest_canvas, 0, 0,
                 dest_width, dest_height,
                 source_dc, 0, 0, width, height,
                 SRCCOPY);
  ::DeleteDC(source_dc);
}
