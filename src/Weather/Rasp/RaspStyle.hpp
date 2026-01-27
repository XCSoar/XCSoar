// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct ColorRamp;
struct ColorRampAlpha;

/**
 * Defines the rendering style for a RASP weather map.
 *
 * At least one of color_ramp (RGB, no transparency) or color_ramp_alpha (RGBA with
 * transparency) must be set. RGBA has preference when both are set.
 */
struct RaspStyle {
  const char *name;
  const ColorRamp *color_ramp;
  const ColorRampAlpha *color_ramp_alpha;
  unsigned height_scale;
  bool do_water;

  /**
   * Returns true if this style uses an alpha-enabled colormap.
   */
  constexpr bool HasAlpha() const noexcept {
    return color_ramp_alpha != nullptr;
  }
};

extern const RaspStyle rasp_styles[];
extern const RaspStyle rasp_colormaps_general[];
