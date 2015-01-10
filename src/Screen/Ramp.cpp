/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

static RGB8Color
Interpolate(short h, const ColorRamp *c1, const ColorRamp *c2,
            unsigned char interp_levels)
{
  if (interp_levels == 0)
    return c1->ToRGB8Color();

  unsigned short is =  1 << interp_levels;
  unsigned short f = (unsigned short)(h - c1->h) * is
                   / (unsigned short)(c2->h - c1->h);
  unsigned short of = is - f;

  return RGB8Color((f * c2->r + of * c1->r) >> interp_levels,
                   (f * c2->g + of * c1->g) >> interp_levels,
                   (f * c2->b + of * c1->b) >> interp_levels);
}

RGB8Color
ColorRampLookup(const short h,
                const ColorRamp* ramp_colors,
                const int numramp,
                const unsigned char interp_levels)
{
  assert(ramp_colors != nullptr);
  assert(numramp >= 2);

  // Check if "h" is above the defined range
  ColorRamp last = ramp_colors[numramp - 1];
  if (h >= last.h)
    return last.ToRGB8Color();

  // Iterate over color ramp control points and find the
  // point above and below "h"
  const ColorRamp *c1 = ramp_colors + numramp - 2;
  const ColorRamp *c2 = c1 + 1;
  while (c1 >= ramp_colors) {
    assert(c1->h < c2->h);

    if (h >= c1->h)
      // Found the two control points -> Interpolate and return the color
      return Interpolate(h, c1, c2, interp_levels);

    c2 = c1;
    c1--;
  }

  // Check if "h" is below the defined range
  ColorRamp first = ramp_colors[0];
  assert(h <= first.h);
  return first.ToRGB8Color();
}

RGB8Color
ColorRamp::ToRGB8Color() const
{
  return RGB8Color(r, g, b);
}
