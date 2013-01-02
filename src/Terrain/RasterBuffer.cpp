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

#include "Terrain/RasterBuffer.hpp"
#include "Math/FastMath.h"

#include <algorithm>
#include <assert.h>

void
RasterBuffer::Resize(unsigned _width, unsigned _height)
{
  assert(_width > 0 && _height > 0);

  data.GrowDiscard(_width, _height);
}

short
RasterBuffer::GetInterpolated(unsigned lx, unsigned ly,
                               unsigned ix, unsigned iy) const
{
  assert(IsDefined());
  assert(lx < GetWidth());
  assert(ly < GetHeight());
  assert(ix < 0x100);
  assert(iy < 0x100);

  // perform piecewise linear interpolation
  const unsigned int dx = (lx == GetWidth() - 1) ? 0 : 1;
  const unsigned int dy = (ly == GetHeight() - 1) ? 0 : GetWidth();
  const short *tm = GetDataAt(lx, ly);

  if (IsSpecial(*tm) || IsSpecial(tm[dx]) ||
      IsSpecial(tm[dy]) || IsSpecial(tm[dx + dy]))
    return *tm;

  unsigned kx = 0x100 - ix;
  unsigned ky = 0x100 - iy;

  return (*tm * kx * ky + tm[dx] * ix * ky + tm[dy] * kx * iy + tm[dx + dy] * ix * iy) >> 16;
}

short
RasterBuffer::GetInterpolated(unsigned lx, unsigned ly) const
{
  // check x in range, and decompose fraction part
  const unsigned int ix = CombinedDivAndMod(lx);
  if (lx >= GetWidth())
    return TERRAIN_INVALID;

  // check y in range, and decompose fraction part
  const unsigned int iy = CombinedDivAndMod(ly);
  if (ly >= GetHeight())
    return TERRAIN_INVALID;

  return GetInterpolated(lx, ly, ix, iy);
}

/**
 * This class implements an algorithm to traverse pixels quickly with
 * only integer addition, no multiplication and division.
 */
class PixelIterator
{
  unsigned src_increment, src_counter;
  unsigned dest_increment, dest_counter;

public:
  PixelIterator(unsigned src_size, unsigned dest_size)
    :src_increment(dest_size), src_counter(0),
     dest_increment(src_size), dest_counter(0) {}

  /**
   * @return the number of source pixels to skip
   */
  unsigned Next() {
    if (dest_counter < src_counter) {
      dest_counter += dest_increment;
      return 0;
    }

    dest_counter += dest_increment;

    unsigned n = 0;

    /* this loop is inefficient with large dest_increment values */
    while (src_counter + src_increment <= dest_counter) {
      src_counter += src_increment;
      ++n;
    }

    return n;
  }
};

void
RasterBuffer::ScanHorizontalLine(unsigned ax, unsigned bx, unsigned y,
                                 short *gcc_restrict buffer, unsigned size,
                                 bool interpolate) const
{
  assert(ax < GetFineWidth());
  assert(bx < GetFineWidth());
  assert(y < GetFineHeight());
  assert(buffer != NULL);
  assert(size > 0);

  if (size == 1) {
    *buffer = Get(ax >> 8, y >> 8);
    return;
  }

  const int dx = bx - ax;
  /* disable interpolation when an output pixel is larger than two
     pixels in our buffer; the factor of two should account for the Y
     axis, which can have a different scale, making the factor some
     sort of ugly kludge to avoid horizontal shading stripes */
  if (interpolate && (unsigned)abs(dx) < (2 * size << 8u)) {
    /* interpolate */

    unsigned cy = y;
    const unsigned int iy = CombinedDivAndMod(cy);

    --size;
    for (int i = 0; (unsigned)i <= size; ++i) {
      unsigned cx = ax + (i * dx) / (int)size;
      const unsigned int ix = CombinedDivAndMod(cx);

      *buffer++ = GetInterpolated(cx, cy, ix, iy);
    }
  } else if (gcc_likely(dx > 0)) {
    /* no interpolation needed, forward scan */

    const short *gcc_restrict src = GetDataAt(ax >> 8, y >> 8);

    PixelIterator iterator(dx >> 8, size);
    short *gcc_restrict end = buffer + size;
    while (true) {
      *buffer++ = *src;
      if (buffer >= end)
        break;
      src += iterator.Next();
    }
  } else {
    /* no interpolation needed */

    const short *gcc_restrict src = GetDataAt(0, y >> 8);

    --size;
    for (int i = 0; (unsigned)i <= size; ++i) {
      unsigned cx = ax + (i * dx) / (int)size;

      *buffer++ = src[cx >> 8];
    }
  }
}

void
RasterBuffer::ScanLine(unsigned ax, unsigned ay, unsigned bx, unsigned by,
                       short *gcc_restrict buffer,
                       unsigned size, bool interpolate) const
{
  assert(ax < GetFineWidth());
  assert(ay < GetFineHeight());
  assert(bx < GetFineWidth());
  assert(by < GetFineHeight());
  assert(buffer != NULL);
  assert(size > 0);

  if (ay == by) {
    ScanHorizontalLine(ax, bx, ay, buffer, size, interpolate);
    return;
  }

  if (size == 1) {
    *buffer = Get(ax >> 8, ay >> 8);
    return;
  }

  --size;
  const int dx = bx - ax, dy = by - ay;
  /* disable interpolation when an output pixel is larger than two
     pixels in our buffer; the factor of two should account for the Y
     axis, which can have a different scale, making the factor some
     sort of ugly kludge to avoid horizontal shading stripes */
  if (interpolate && (unsigned)(abs(dx) + abs(dy)) < (2 * size << 8u)) {
    /* interpolate */

    for (int i = 0; (unsigned)i <= size; ++i) {
      unsigned cx = ax + (i * dx) / (int)size;
      unsigned cy = ay + (i * dy) / (int)size;

      const unsigned int ix = CombinedDivAndMod(cx);
      const unsigned int iy = CombinedDivAndMod(cy);

      *buffer++ = GetInterpolated(cx, cy, ix, iy);
    }
  } else {
    /* no interpolation needed */

    for (int i = 0; (unsigned)i <= size; ++i) {
      unsigned cx = ax + (i * dx) / (int)size;
      unsigned cy = ay + (i * dy) / (int)size;

      *buffer++ = Get(cx >> 8, cy >> 8);
    }
  }
}

void
RasterBuffer::ScanLineChecked(unsigned ax, unsigned ay,
                              unsigned bx, unsigned by,
                              short *buffer, unsigned size,
                              bool interpolate) const
{
  if (ax >= GetFineWidth())
    ax = GetFineWidth() - 1;

  if (ay >= GetFineHeight())
    ay = GetFineHeight() - 1;

  if (bx >= GetFineWidth())
    bx = GetFineWidth() - 1;

  if (by >= GetFineHeight())
    by = GetFineHeight() - 1;

  ScanLine(ax, ay, bx, by, buffer, size, interpolate);
}

short
RasterBuffer::GetMaximum() const
{
  return IsDefined() ? *std::max_element(data.begin(), data.end()) : 0;
}
