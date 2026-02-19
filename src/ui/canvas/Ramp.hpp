// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PortableColor.hpp"

class RGB8Color;

/* A color ramp entry */
struct ColorRampEntry {
  short h;
  RGB8Color color;
};

/**
 * A color ramp entry with alpha channel support.
 */
struct ColorRampEntryAlpha {
  short h;
  RGBA8Color color;
};

/**
 * A color ramp that maps a range of integer values to RGB colors.
 * The actual colors are defined by a table of ColorRampEntry objects
 */
struct ColorRamp{
  bool has_alpha;
  short num_entries;
  const ColorRampEntry *ramp_table;
  const ColorRampEntryAlpha *ramp_table_alpha;
};

[[gnu::pure]]
RGB8Color
ColorRampLookup(int h,
                const ColorRamp* ramp_colors,
                unsigned numramp, unsigned interp_bits=6);


[[gnu::pure]]
RGBA8Color
ColorRampLookupAlpha(int h,
                     const ColorRamp* ramp_colors,
                     unsigned numramp, unsigned interp_bits=6);
