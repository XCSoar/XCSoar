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
   corrected_width(CorrectedWidth(nWidth))
{
  assert(nWidth > 0);
  assert(nHeight > 0);

  Uint32 rmask, gmask, bmask, amask;
  int depth;

#ifdef HAVE_GLES
  rmask = 0x0000f800;
  gmask = 0x000007e0;
  bmask = 0x0000001f;
  depth = 16;
#else
  rmask = 0x00ff0000;
  gmask = 0x0000ff00;
  bmask = 0x000000ff;
  depth = 32;
#endif
  amask = 0x00000000;

  assert(sizeof(BGRColor) * 8 == depth);

  surface = ::SDL_CreateRGBSurface(SDL_SWSURFACE, corrected_width, height,
                                   depth, rmask, gmask, bmask, amask);
  assert(!SDL_MUSTLOCK(surface));

  buffer = (BGRColor *)surface->pixels;
}

RawBitmap::~RawBitmap()
{
  ::SDL_FreeSurface(surface);
}

void
RawBitmap::StretchTo(unsigned width, unsigned height,
                     Canvas &dest_canvas,
                     unsigned dest_width, unsigned dest_height) const
{
  Canvas src_canvas(surface);
  dest_canvas.Stretch(0, 0, dest_width, dest_height,
                      src_canvas, 0, 0, width, height);
}
