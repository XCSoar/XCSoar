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

#include "Terrain/RasterBuffer.hpp"

#include <algorithm>
#include <cassert>
#include <stdlib.h>

void
RasterBuffer::Resize(RasterLocation _size) noexcept
{
  assert(_size.x > 0);
  assert(_size.y > 0);

  data.GrowDiscard(_size.x, _size.y);
}

TerrainHeight
RasterBuffer::GetInterpolated(unsigned lx, unsigned ly,
                              unsigned ix, unsigned iy) const noexcept
{
  assert(IsDefined());
  assert(lx < GetSize().x);
  assert(ly < GetSize().y);
  assert(ix < 0x100);
  assert(iy < 0x100);

  // perform piecewise linear interpolation
  const unsigned int dx = (lx == GetSize().x - 1) ? 0 : 1;
  const unsigned int dy = (ly == GetSize().y - 1) ? 0 : GetSize().x;
  const TerrainHeight *tm = GetDataAt({lx, ly});

  if (tm->IsSpecial() || tm[dx].IsSpecial() ||
      tm[dy].IsSpecial() || tm[dx + dy].IsSpecial())
    return *tm;

  unsigned kx = 0x100 - ix;
  unsigned ky = 0x100 - iy;

  return TerrainHeight((tm->GetValue() * kx * ky
                        + tm[dx].GetValue() * ix * ky
                        + tm[dy].GetValue() * kx * iy
                        + tm[dx + dy].GetValue() * ix * iy) >> 16);
}

TerrainHeight
RasterBuffer::GetInterpolated(RasterLocation p) const noexcept
{
  // check x in range, and decompose fraction part
  const auto [px, ix] = RasterTraits::CalcSubpixel(p.x);
  if (px >= GetSize().x)
    return TerrainHeight::Invalid();

  // check y in range, and decompose fraction part
  const auto [py, iy] = RasterTraits::CalcSubpixel(p.y);
  if (py >= GetSize().y)
    return TerrainHeight::Invalid();

  return GetInterpolated(px, py, ix, iy);
}

/**
 * This class implements an algorithm to traverse pixels quickly with
 * only integer addition, no multiplication and division.
 */
class PixelIterator
{
  int error = 0;
  int src_increment;
  int dest_increment;

public:
  constexpr PixelIterator(unsigned src_size, unsigned dest_size) noexcept
    :src_increment(dest_size),
     dest_increment(src_size) {}

  /**
   * @return the number of source pixels to skip
   */
  constexpr unsigned Next() noexcept {
    if (error < 0) {
      error += dest_increment;
      return 0;
    }

    error += dest_increment;

    unsigned n = 0;

    /* this loop is inefficient with large dest_increment values */
    while (error >= src_increment) {
      error -= src_increment;
      ++n;
    }

    return n;
  }
};

[[gnu::hot]]
void
RasterBuffer::ScanHorizontalLine(unsigned ax, unsigned bx, unsigned y,
                                 TerrainHeight *gcc_restrict buffer, unsigned size,
                                 bool interpolate) const noexcept
{
  assert(ax < GetFineSize().x);
  assert(bx < GetFineSize().x);
  assert(y < GetFineSize().y);
  assert(buffer != nullptr);
  assert(size > 0);

  if (size == 1) {
    *buffer = Get({
        ax >> RasterTraits::SUBPIXEL_BITS,
        y >> RasterTraits::SUBPIXEL_BITS,
      });
    return;
  }

  const int dx = bx - ax;
  /* disable interpolation when an output pixel is larger than two
     pixels in our buffer; the factor of two should account for the Y
     axis, which can have a different scale, making the factor some
     sort of ugly kludge to avoid horizontal shading stripes */
  if (interpolate &&
      (unsigned)abs(dx) < (2 * size << RasterTraits::SUBPIXEL_BITS)) {
    /* interpolate */

    const auto [cy, iy] = RasterTraits::CalcSubpixel(y);

    --size;
    for (int i = 0; (unsigned)i <= size; ++i) {
      const auto [cx, ix] =
        RasterTraits::CalcSubpixel(ax + (i * dx) / (int)size);

      *buffer++ = GetInterpolated(cx, cy, ix, iy);
    }
  } else if (gcc_likely(dx > 0)) {
    /* no interpolation needed, forward scan */

    const TerrainHeight *gcc_restrict src =
      GetDataAt({
          ax >> RasterTraits::SUBPIXEL_BITS,
          y >> RasterTraits::SUBPIXEL_BITS,
        });

    PixelIterator iterator(dx >> RasterTraits::SUBPIXEL_BITS, size);
    TerrainHeight *gcc_restrict end = buffer + size;
    while (buffer != end) {
      *buffer++ = *src;
      src += iterator.Next();
    }
  } else {
    /* no interpolation needed */

    const TerrainHeight *gcc_restrict src =
      GetDataAt({0, y >> RasterTraits::SUBPIXEL_BITS});

    --size;
    for (int i = 0; (unsigned)i <= size; ++i) {
      unsigned cx = ax + (i * dx) / (int)size;

      *buffer++ = src[cx >> RasterTraits::SUBPIXEL_BITS];
    }
  }
}

[[gnu::hot]]
void
RasterBuffer::ScanLine(RasterLocation a, RasterLocation b,
                       TerrainHeight *gcc_restrict buffer,
                       unsigned size, bool interpolate) const noexcept
{
  assert(a.x < GetFineSize().x);
  assert(a.y < GetFineSize().y);
  assert(b.x < GetFineSize().x);
  assert(b.y < GetFineSize().y);
  assert(buffer != nullptr);
  assert(size > 0);

  if (a.y == b.y) {
    ScanHorizontalLine(a.x, b.x, a.y, buffer, size, interpolate);
    return;
  }

  if (size == 1) {
    *buffer = Get(a >> RasterTraits::SUBPIXEL_BITS);
    return;
  }

  --size;
  const IntPoint2D d(b.x - a.x, b.y - a.y);
  /* disable interpolation when an output pixel is larger than two
     pixels in our buffer; the factor of two should account for the Y
     axis, which can have a different scale, making the factor some
     sort of ugly kludge to avoid horizontal shading stripes */
  if (interpolate &&
      (unsigned)(abs(d.x) + abs(d.y)) < (2 * size << RasterTraits::SUBPIXEL_BITS)) {
    /* interpolate */

    for (int i = 0; (unsigned)i <= size; ++i) {
      const auto [cx, ix] =
        RasterTraits::CalcSubpixel(a.x + (i * d.x) / (int)size);
      const auto [cy, iy] =
        RasterTraits::CalcSubpixel(a.y + (i * d.y) / (int)size);

      *buffer++ = GetInterpolated(cx, cy, ix, iy);
    }
  } else {
    /* no interpolation needed */

    for (int i = 0; (unsigned)i <= size; ++i) {
      const RasterLocation c(a.x + (i * d.x) / (int)size,
                             a.y + (i * d.y) / (int)size);

      *buffer++ = Get(c >> RasterTraits::SUBPIXEL_BITS);
    }
  }
}

void
RasterBuffer::ScanLineChecked(RasterLocation a, RasterLocation b,
                              TerrainHeight *buffer, unsigned size,
                              bool interpolate) const noexcept
{
  if (a.x >= GetFineSize().x)
    a.x = GetFineSize().x - 1;

  if (a.y >= GetFineSize().y)
    a.y = GetFineSize().y - 1;

  if (b.x >= GetFineSize().x)
    b.x = GetFineSize().x - 1;

  if (b.y >= GetFineSize().y)
    b.y = GetFineSize().y - 1;

  ScanLine(a, b, buffer, size, interpolate);
}

TerrainHeight
RasterBuffer::GetMaximum() const noexcept
{
  return IsDefined()
    ? *std::max_element(data.begin(), data.end(),
                        [](TerrainHeight a, TerrainHeight b) {
                          return a.GetValue() < b.GetValue();
                        })
    : TerrainHeight(0);
}
