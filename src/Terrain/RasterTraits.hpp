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
 * The fixed-point fractional part of sub-pixel coordinates.
 */
constexpr unsigned SUBPIXEL_BITS = 8;

constexpr unsigned SUBPIXEL_MASK = ~((~0u) << SUBPIXEL_BITS);

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
