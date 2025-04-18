// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../RawBitmap.hpp"
#include "Canvas.hpp"

#include <cassert>

/**
 * Returns minimum width that is greater then the given width and
 * that is acceptable as image width (not all numbers are acceptable)
 */
static constexpr unsigned
CorrectedWidth(unsigned nWidth) noexcept
{
  return ((nWidth + 3) / 4) * 4;
}

RawBitmap::RawBitmap(PixelSize _size) noexcept
  :size(_size),
   corrected_width(CorrectedWidth(size.width)),
   buffer(new RawColor[corrected_width * size.height])
{
  assert(size.width > 0);
  assert(size.height > 0);

  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = corrected_width;
  bi.bmiHeader.biHeight = size.height;
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

RawBitmap::~RawBitmap() noexcept
{
  ::DeleteObject(bitmap);
}

void
RawBitmap::StretchTo(PixelSize src_size,
                     Canvas &dest_canvas, PixelSize dest_size,
                     bool transparent_white) const noexcept
{
  HDC source_dc = ::CreateCompatibleDC(dest_canvas);
  ::SelectObject(source_dc, bitmap);
  if (transparent_white)
    ::TransparentBlt(dest_canvas, 0, 0, dest_size.width, dest_size.height,
                     source_dc, 0, 0, src_size.width, src_size.height,
                     COLOR_WHITE);
  else
    ::StretchBlt(dest_canvas, 0, 0, dest_size.width, dest_size.height,
                 source_dc, 0, 0, src_size.width, src_size.height,
                 SRCCOPY);
  ::DeleteDC(source_dc);
}
