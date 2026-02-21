// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ColorMap.hpp"

/**
 * Defines the rendering style for a RASP weather map.
 *
 * The primary data is a physical color map (value -> color) plus
 * a value transform that connects physical values to the integer
 * domain used by the rendering pipeline.
 * color_map_alpha should be used if provided and alpha blending is available
 * color_map can define a non-alpha optimized map for when alpha is 
 * not supported/used
 */
struct RaspStyle {
  const char *name;

  ColorMap color_map;
  ColorMap color_map_alpha;
  
  /**
   * Linear value transform from physical to rendering domain:
   * h = physical_value * scale + offset
   */
  float scale;
  float offset;

  unsigned height_scale;
  bool do_water;

  /**
   * Returns true if this style uses an alpha-enabled colormap.
   */
  bool HasAlpha() const noexcept {
    return color_map_alpha.num_points > 0;
  }
};

extern const RaspStyle rasp_styles[];
extern const RaspStyle rasp_colormaps_general[];

/**
 * Look up the rendering style for a RASP weather field by name.
 * Falls back to matching the last 3 characters against
 * rasp_colormaps_general, then returns a default style.
 */
[[gnu::pure]]
const RaspStyle &
LookupWeatherTerrainStyle(const char *name);
