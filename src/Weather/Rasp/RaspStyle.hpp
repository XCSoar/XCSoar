// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ColorMap.hpp"
#include "Units/Group.hpp"

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
   * Physical quantity used to display a field value in user units.
   * UnitGroup::NONE means the field is unitless and the colormap
   * value is shown as a plain number.  This does not affect colour
   * rendering, only value labels.
   */
  UnitGroup unit_group = UnitGroup::NONE;

  /**
   * Linear transform from the colormap value-space (the values in
   * #color_map) to the canonical system unit of #unit_group:
   *
   *   system_value = colormap_value * value_scale + value_offset
   *
   * e.g. cm/s -> m/s, knots -> m/s, or degrees Celsius -> Kelvin.
   * Defaults to the identity transform, used when the colormap is
   * already authored in the system unit.
   */
  float value_scale = 1;
  float value_offset = 0;

  /**
   * Returns true if this style uses an alpha-enabled colormap.
   */
  bool HasAlpha() const noexcept {
    return color_map_alpha.num_points > 0;
  }

  /**
   * Convert a value from the colormap value-space into the canonical
   * system unit for #unit_group (e.g. for use with Units::ToUserUnit).
   */
  [[gnu::pure]]
  double ToSystemValue(double colormap_value) const noexcept {
    return colormap_value * value_scale + value_offset;
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
