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

#include "Screen/Ramp.hpp"

#include <assert.h>
#include <stddef.h>

Color
ColorRampLookup(const short h,
                const ColorRamp* ramp_colors,
                const int numramp,
                const unsigned char interp_levels)
{
  assert(ramp_colors != NULL);
  assert(numramp >= 2);

  unsigned short f, of;
  unsigned short is = 1<<interp_levels;

  // gone past end, so use last color
  if (h >= ramp_colors[numramp - 1].h) {
    return Color(ramp_colors[numramp-1].r,
                 ramp_colors[numramp-1].g,
                 ramp_colors[numramp-1].b);
  }
  for (int i = numramp - 2; i >= 0; i--) {
    assert(ramp_colors[i].h < ramp_colors[i + 1].h);

    if (h >= ramp_colors[i].h) {
      if (interp_levels) {
        f = (unsigned short)(h - ramp_colors[i].h) * is
          / (unsigned short)(ramp_colors[i + 1].h - ramp_colors[i].h);
        of = is - f;

        return Color((f * ramp_colors[i + 1].r + of * ramp_colors[i].r) >> interp_levels,
                     (f * ramp_colors[i + 1].g + of * ramp_colors[i].g) >> interp_levels,
                     (f * ramp_colors[i + 1].b + of * ramp_colors[i].b) >> interp_levels);
      } else {
        return Color(ramp_colors[i].r, ramp_colors[i].g, ramp_colors[i].b);
      }
    }
  }

  // check if h lower than lowest
  assert(h <= ramp_colors[0].h);

  return Color(ramp_colors[0].r, ramp_colors[0].g, ramp_colors[0].b);
}
