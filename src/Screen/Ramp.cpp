/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

#include "Screen/Ramp.hpp"
#include "PortableColor.hpp"

#include <assert.h>

RGB8Color
ColorRampLookup(const short h,
                const ColorRamp* ramp_colors,
                const int numramp,
                const unsigned char interp_levels)
{
  assert(ramp_colors != nullptr);
  assert(numramp >= 2);

  unsigned short f, of;
  unsigned short is = 1<<interp_levels;

  // gone past end, so use last color
  ColorRamp last = ramp_colors[numramp - 1];
  if (h >= last.h)
    return RGB8Color(last.r, last.g, last.b);

  const ColorRamp *c1 = ramp_colors + numramp - 2;
  const ColorRamp *c2 = c1 + 1;

  while (c1 >= ramp_colors) {
    assert(c1->h < c2->h);

    if (h >= c1->h) {
      if (interp_levels == 0)
        return RGB8Color(c1->r, c1->g, c1->b);

      f = (unsigned short)(h - c1->h) * is
        / (unsigned short)(c2->h - c1->h);
      of = is - f;

      return RGB8Color((f * c2->r + of * c1->r) >> interp_levels,
                       (f * c2->g + of * c1->g) >> interp_levels,
                       (f * c2->b + of * c1->b) >> interp_levels);
    }

    c2 = c1;
    c1--;
  }

  // check if h lower than lowest
  ColorRamp first = ramp_colors[0];
  assert(h <= first.h);

  return RGB8Color(first.r, first.g, first.b);
}
