// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Point2D.hpp"

#include <algorithm>

namespace DisplayMetrics {

/**
 * Convert pixel count and physical span (millimetres) to DPI.
 * Uses 25.4 mm per inch (254/10).  Returns 0 if @a mm is zero.
 */
[[nodiscard]] constexpr unsigned
MMToDPI(unsigned pixels, unsigned mm) noexcept
{
  if (mm == 0)
    return 0;
  return pixels * 254 / (mm * 10);
}

/**
 * Physical DPI from pixel dimensions and physical span in millimetres.
 * Returns {0,0} when any dimension is zero.
 */
[[nodiscard]] constexpr UnsignedPoint2D
PhysicalDpiFromSizeMm(unsigned width, unsigned height,
                      unsigned width_mm, unsigned height_mm) noexcept
{
  if (width == 0 || height == 0 || width_mm == 0 || height_mm == 0)
    return {0, 0};
  return {
    MMToDPI(width, width_mm),
    MMToDPI(height, height_mm),
  };
}

/**
 * Reference DPI paired with compositor content scale so effective DPI
 * matches typical X11 Xft.dpi (120) when scaling logical vs physical.
 */
inline constexpr unsigned CONTENT_SCALE_DPI_REFERENCE = 120U;

/**
 * DPI used for fonts and pens when content_scale is greater than 1
 * (logical vs buffer scale) so we do not double-scale.
 */
[[nodiscard]] constexpr UnsignedPoint2D
EffectiveDpiForContentScale(UnsignedPoint2D physical_dpi,
                            unsigned content_scale) noexcept
{
  if (content_scale <= 1)
    return physical_dpi;
  return {
    std::clamp(physical_dpi.x * CONTENT_SCALE_DPI_REFERENCE /
                 (content_scale * 100),
               1u, physical_dpi.x),
    std::clamp(physical_dpi.y * CONTENT_SCALE_DPI_REFERENCE /
                 (content_scale * 100),
               1u, physical_dpi.y),
  };
}

/**
 * Touch control row height in the same logical space as
 * EffectiveDpiForContentScale when content_scale is greater than 1.
 */
[[nodiscard]] constexpr unsigned
TouchControlHeightForContentScale(unsigned scaled_height,
                                  unsigned content_scale) noexcept
{
  if (content_scale <= 1)
    return scaled_height;
  return scaled_height / content_scale *
         CONTENT_SCALE_DPI_REFERENCE / 100;
}

} // namespace DisplayMetrics
