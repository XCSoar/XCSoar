// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <utility>

/**
 * This namespace contains information on how the terrain raster works
 * in XCSoar.
 */
namespace RasterTraits {

/**
 * The width and height of the terrain bitmap is shifted by this
 * number of bits to determine the overview size.
 */
constexpr unsigned OVERVIEW_BITS = 4;

constexpr unsigned OVERVIEW_MASK = ~((~0u) << OVERVIEW_BITS);

/**
 * Intermediate grid kept next to the overview.  Four DEM samples
 * become one step sample.  A wide view scans this instead of
 * decoding fine tiles.
 */
constexpr unsigned STEP_BITS = 2;

constexpr unsigned STEP_MASK = ~((~0u) << STEP_BITS);

static_assert(STEP_BITS < OVERVIEW_BITS);

/**
 * The fixed-point fractional part of sub-pixel coordinates.
 */
constexpr unsigned SUBPIXEL_BITS = 8;

constexpr unsigned SUBPIXEL_MASK = ~((~0u) << SUBPIXEL_BITS);

static_assert(OVERVIEW_BITS < SUBPIXEL_BITS);

/**
 * Convert a pixel size to an overview pixel size, rounding down.
 */
constexpr unsigned ToOverview(unsigned x) noexcept {
  return x >> OVERVIEW_BITS;
}

/**
 * Convert a pixel size to an overview pixel size, rounding up.
 */
constexpr unsigned ToOverviewCeil(unsigned x) noexcept {
  return ToOverview(x + OVERVIEW_MASK);
}

constexpr unsigned ToStep(unsigned x) noexcept {
  return x >> STEP_BITS;
}

constexpr unsigned ToStepCeil(unsigned x) noexcept {
  return ToStep(x + STEP_MASK);
}

/**
 * Which pixel inside a coarse block is stored for that block.
 * The middle, so the picture does not jump east or south when a
 * fine tile replaces the coarse sample.
 */
constexpr unsigned SubsamplePhase(unsigned shift) noexcept {
  return shift == 0 ? 0u : 1u << (shift - 1);
}

/**
 * True when one screen sample is at least as large as one step
 * sample, so the cached step grid is as sharp as the display.
 * A zero step size never covers: the caller keeps the fine tiles.
 */
constexpr bool
StepCoversPixel(double pixel_size_m, double step_size_m) noexcept {
  return step_size_m > 0 && pixel_size_m >= step_size_m;
}

/**
 * Isolate the full-pixel value and the subpixel portion from a
 * subpixel value.
 */
constexpr std::pair<unsigned, unsigned>
CalcSubpixel(unsigned fine) noexcept
{
  return {fine >> SUBPIXEL_BITS, fine & SUBPIXEL_MASK};
}

} // namespace RasterTraits
