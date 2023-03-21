// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Ramp.hpp"

#include <cassert>

static constexpr RGB8Color
Interpolate(RGB8Color c1, RGB8Color c2,
            unsigned f, unsigned of,
            unsigned interp_levels)
{
  return RGB8Color((f * c2.Red() + of * c1.Red()) >> interp_levels,
                   (f * c2.Green() + of * c1.Green()) >> interp_levels,
                   (f * c2.Blue() + of * c1.Blue()) >> interp_levels);
}

static RGB8Color
Interpolate(int h, const ColorRamp *c1, const ColorRamp *c2,
            unsigned interp_levels)
{
  if (interp_levels == 0)
    return c1->color;

  unsigned is = 1u << interp_levels;
  unsigned f = unsigned(h - c1->h) * is / unsigned(c2->h - c1->h);
  unsigned of = is - f;

  return Interpolate(c1->color, c2->color, f, of, interp_levels);
}

RGB8Color
ColorRampLookup(const int h,
                const ColorRamp* ramp_colors,
                const unsigned numramp,
                const unsigned interp_levels)
{
  assert(ramp_colors != nullptr);
  assert(numramp >= 2);

  // Check if "h" is above the defined range
  ColorRamp last = ramp_colors[numramp - 1];
  if (h >= last.h)
    return last.color;

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
  return first.color;
}
