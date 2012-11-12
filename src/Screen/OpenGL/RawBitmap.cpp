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

#include "../RawBitmap.hpp"

#include <assert.h>

/**
 * Returns minimum width that is greater then the given width and
 * that is acceptable as image width (not all numbers are acceptable)
 */
static inline UPixelScalar
CorrectedWidth(UPixelScalar nWidth)
{
  return ((nWidth + 3) / 4) * 4;
}

RawBitmap::RawBitmap(UPixelScalar nWidth, UPixelScalar nHeight)
  :width(nWidth), height(nHeight),
   corrected_width(CorrectedWidth(nWidth)),
   texture(new GLTexture(CorrectedWidth(nWidth), nHeight)),
   dirty(true)
{
  assert(nWidth > 0);
  assert(nHeight > 0);

  AddSurfaceListener(*this);

  buffer = new BGRColor[corrected_width * height];
}

RawBitmap::~RawBitmap()
{
  RemoveSurfaceListener(*this);

  delete texture;
  delete[] buffer;
}

void
RawBitmap::surface_created()
{
  if (texture == NULL)
    texture = new GLTexture(corrected_width, height);
}

void
RawBitmap::surface_destroyed()
{
  delete texture;
  texture = NULL;

  dirty = true;
}
