// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PortableColor.hpp"

class RGB8Color;

struct ColorRamp {
  short h;
  RGB8Color color;
};

[[gnu::pure]]
RGB8Color
ColorRampLookup(int h,
                const ColorRamp* ramp_colors,
                unsigned numramp, unsigned interp_bits=6);
