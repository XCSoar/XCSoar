// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Terrain/TerrainSettings.hpp"

#include <algorithm>

void
TerrainRendererSettings::SetDefaults()
{
  enable = true;
  slope_shading = SlopeShading::WIND;
  contrast = 65;
  brightness = 192;
  ramp = 0;
  contours = Contours::OFF;
}

/**
 * Maximum contour height scale (bit shift): stop the contour scaling
 * at this level (contour interval > Mount Everest height)
 */
static constexpr unsigned CONTOUR_HEIGHT_SCALE_MAX = 15;

/**
 * Contour density preset definition:
 * base_scale_offset: added to height_scale to compute the starting
 *   contour_height_scale.  Higher values = coarser base interval.
 * min_spacing_factor: contour interval is at least this many
 *   millimeters at 45 degree slope (approximately).  
 *   Higher value = more thinning when zoomed out.
 */

struct ContourModeParams {
  int base_scale_offset;
  double min_spacing_factor;
};

// Mountains: coarse base interval (256 m at height_scale=4)
static constexpr ContourModeParams CONTOUR_MOUNTAINS{4, 1.5};

// Highlands: medium-coarse base interval (64 m at height_scale=4)
static constexpr ContourModeParams CONTOUR_HIGHLANDS{2, 0.75};

// Lowlands: medium-fine base interval (16 m at height_scale=4)
static constexpr ContourModeParams CONTOUR_LOWLANDS{0, 0.5};

// Superfine: very fine base interval (8 m at height_scale=4)
static constexpr ContourModeParams CONTOUR_SUPERFINE{-1, 0.5};

unsigned
ContourSpacing(Contours contours, unsigned height_scale,
               double pixel_size) noexcept
{
  if (contours == Contours::OFF)
    return 0;

  /* Fixed-spacing modes: return a constant interval (in meters) regardless
     of zoom.  Very-far-zoom suppression is handled in RasterRenderer via
     quantisation_effective == 0. */
  switch (contours) {
  case Contours::FIXED_256:
    return 256u;
  case Contours::FIXED_128:
    return 128u;
  case Contours::FIXED_64:
    return 64u;
  default:
    break;
  }

  const auto &params = [&]() -> const ContourModeParams & {
    switch (contours) {
    case Contours::HIGHLANDS:
      return CONTOUR_HIGHLANDS;
    case Contours::LOWLANDS:
      return CONTOUR_LOWLANDS;
    case Contours::SUPERFINE:
      return CONTOUR_SUPERFINE;
    case Contours::MOUNTAINS:
    default:
      return CONTOUR_MOUNTAINS;
    }
  }();

  unsigned contour_height_scale =
    std::min((unsigned)std::max(0,
                                (int)height_scale
                                + params.base_scale_offset),
             CONTOUR_HEIGHT_SCALE_MAX);

  unsigned interval = 1u << contour_height_scale;
  while (interval < (unsigned)(pixel_size
                               * params.min_spacing_factor)
         && contour_height_scale < CONTOUR_HEIGHT_SCALE_MAX) {
    interval <<= 1;
    contour_height_scale++;
  }

  return interval;
}
