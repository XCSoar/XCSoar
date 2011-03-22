/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
RasterBuffer::resize(unsigned _width, unsigned _height)
{
  assert(_width > 0 && _height > 0);

  data.GrowDiscard(_width, _height);
}

short
RasterBuffer::get_interpolated(unsigned lx, unsigned ly,
                               unsigned ix, unsigned iy) const
{
  assert(defined());
  assert(lx < get_width());
  assert(ly < get_height());
  assert(ix < 0x100);
  assert(iy < 0x100);

  // perform piecewise linear interpolation
  const unsigned int dx = (lx == get_width() - 1) ? 0 : 1;
  const unsigned int dy = (ly == get_height() - 1) ? 0 : get_width();
  const short *tm = get_data_at(lx, ly);

  if (is_special(*tm) || is_special(tm[dx]) ||
      is_special(tm[dy]) || is_special(tm[dx + dy]))
    return *tm;

  unsigned kx = 0x100 - ix;
  unsigned ky = 0x100 - iy;

  return (*tm * kx * ky + tm[dx] * ix * ky + tm[dy] * kx * iy + tm[dx + dy] * ix * iy) >> 16;
}

short
RasterBuffer::get_interpolated(unsigned lx, unsigned ly) const
{
  // check x in range, and decompose fraction part
  const unsigned int ix = CombinedDivAndMod(lx);
  if (lx >= get_width())
    return TERRAIN_INVALID;

  // check y in range, and decompose fraction part
  const unsigned int iy = CombinedDivAndMod(ly);
  if (ly >= get_height())
    return TERRAIN_INVALID;

  return get_interpolated(lx, ly, ix, iy);
}

short
RasterBuffer::get_max() const
{
  return defined() ? *std::max_element(data.begin(), data.end()) : 0;
}
