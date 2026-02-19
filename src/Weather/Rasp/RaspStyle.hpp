// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct ColorRamp;

/**
 * Defines the rendering style for a RASP weather map.
 *
 * The color_ramp wrapper contains both RGB and optional RGBA entry tables.
 * Use HasAlpha() to check if the alpha table is available.
 */
struct RaspStyle {
  const char *name;
  const ColorRamp *color_ramp;
  unsigned height_scale;
  bool do_water;

  /**
   * Returns true if this style uses an alpha-enabled colormap.
   */
  bool HasAlpha() const noexcept;
};

extern const RaspStyle rasp_styles[];
extern const RaspStyle rasp_colormaps_general[];
