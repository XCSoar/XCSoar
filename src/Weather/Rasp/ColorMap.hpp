// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/PortableColor.hpp"
#include "ui/canvas/Ramp.hpp"

#include <cstdint>
#include <vector>

/**
 * A color control point defined in value space (not file encoding space).
 * Maps a value (e.g., temperature in °C) to a color with alpha channel.
 * Alpha is ignored when used for non-alpha color ramps.
 */
struct ColorPoint {
  float value;
  RGBA8Color color;
};

/**
 * A color map that defines the mapping from physical values to colors,
 * independent of data/file encoding. ColorRamp tables for rendering
 * are generated from this by linear transformation.
 * Points must be sorted by ascending value.
 */
struct ColorMap {
  const ColorPoint *points;
  unsigned num_points;
};

/**
 * Result of materializing a ColorMap into a rendering-domain
 * ColorRamp by applying a linear value transform.
 */
struct MaterializedColorRamp {
  std::vector<ColorRampEntry> entries;
  std::vector<ColorRampEntryAlpha> entries_alpha;
  bool has_alpha;
  uint32_t hash;

  /**
   * Build a transient ColorRamp struct pointing into our vectors.
   * Only valid while this MaterializedColorRamp is alive.
   */
  [[gnu::pure]]
  ColorRamp GetColorRamp() const noexcept;
};

/**
 * Materialize a physical color map into a rendering-domain color ramp
 * by applying a linear value transform: h = physical_value * scale + offset
 *
 * @param color_map       RGB color map (physical value → color)
 * @param color_map_alpha RGBA color map, or empty ColorMap
 *                        if no alpha is needed
 * @param scale           multiplier for the value transform
 * @param offset          additive offset for the value transform
 * @param height_scale    included in hash for cache invalidation
 * @param do_water        included in hash for cache invalidation
 */
MaterializedColorRamp
MaterializeColorRamp(const ColorMap &color_map,
                     const ColorMap &color_map_alpha,
                     float scale, float offset,
                     unsigned height_scale,
                     bool do_water) noexcept;
