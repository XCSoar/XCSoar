// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cassert>

/**
 * Density of RASP contour lines. Each step doubles or halves the
 * number of lines relative to REGULAR (~16 lines across the field
 * range).
 */
enum class ContourDensity : unsigned {
  OFF = 0,
  WIDE = 1,      ///< ~8 lines (spacing × 2)
  REGULAR = 2,   ///< ~16 lines (baseline)
  FINE = 3,      ///< ~32 lines (spacing / 2)
  SUPERFINE = 4, ///< ~64 lines (spacing / 4)
  COUNT
};

/**
 * Convert ContourDensity + a RASP height_scale into a contour_spacing
 * value suitable for RasterRenderer::GenerateImage(). Returns 0 when
 * contours are disabled.
 */
[[gnu::const]] inline unsigned
ContourSpacing(ContourDensity density, unsigned height_scale) noexcept
{
  switch (density) {
  case ContourDensity::OFF:       return 0;
  case ContourDensity::WIDE:      return 1u << (height_scale + 5);
  case ContourDensity::REGULAR:   return 1u << (height_scale + 4);
  case ContourDensity::FINE:      return 1u << (height_scale + 3);
  case ContourDensity::SUPERFINE: return 1u << (height_scale + 2);
  case ContourDensity::COUNT:
    assert(false);
  }
  return 0;
}
