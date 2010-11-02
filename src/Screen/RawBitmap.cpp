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

#include "Screen/RawBitmap.hpp"

#include <assert.h>
#include <algorithm>

/**
 * Returns minimum width that is greater then the given width and
 * that is acceptable as image width (not all numbers are acceptable)
 */
static inline unsigned
CorrectedWidth(unsigned nWidth)
{
  return ((nWidth + 3) / 4) * 4;
}

RawBitmap::RawBitmap(unsigned nWidth, unsigned nHeight, const Color clr)
  :width(nWidth), height(nHeight),
   corrected_width(CorrectedWidth(nWidth))
{
  assert(nWidth > 0);
  assert(nHeight > 0);

#ifdef ENABLE_SDL
  Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
#else
  rmask = 0x00ff0000;
  gmask = 0x0000ff00;
  bmask = 0x000000ff;
#endif
  amask = 0x00000000;

  surface = ::SDL_CreateRGBSurface(SDL_SWSURFACE, corrected_width, height, 24,
                                   rmask, gmask, bmask, amask);
  assert(!SDL_MUSTLOCK(surface));

  buffer = (BGRColor *)surface->pixels;
#else /* !ENABLE_SDL */
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

#if defined(_WIN32_WCE) && _WIN32_WCE < 0x0400
  /* StretchDIBits() is bugged on PPC2002, workaround follows */
  VOID *pvBits;
  HDC hDC = ::GetDC(NULL);
  bitmap = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
  ::ReleaseDC(NULL, hDC);
  buffer = (BGRColor *)pvBits;
#else
  buffer = new BGRColor[corrected_width * height];
#endif
#endif /* !ENABLE_SDL */

  std::fill(buffer, buffer + corrected_width * height,
            BGRColor(clr.blue(), clr.green(), clr.red()));
}

RawBitmap::~RawBitmap()
{
#ifdef ENABLE_SDL
  ::SDL_FreeSurface(surface);
#elif defined(_WIN32_WCE) && _WIN32_WCE < 0x0400
  ::DeleteObject(bitmap);
#else
  delete[] buffer;
#endif
}
