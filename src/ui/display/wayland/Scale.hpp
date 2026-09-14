// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Size.hpp"

#include <cstdint>

namespace Wayland {

/**
 * Wayland fractional scale is an integer in 120ths of 100%:
 * 120 = 100%, 240 = 200%.  Physical = round(logical * scale / 120).
 */
constexpr unsigned SCALE_100 = 120;
constexpr unsigned SCALE_125 = 150;
constexpr unsigned SCALE_150 = 180;
constexpr unsigned SCALE_200 = 240;

[[gnu::const]]
constexpr unsigned
ToPhysicalPixels(unsigned logical_px, unsigned scale_120ths) noexcept
{
  return (unsigned)(((std::uint64_t)logical_px * scale_120ths +
                     SCALE_100 / 2) / SCALE_100);
}

[[gnu::const]]
constexpr PixelSize
ToPhysicalSize(PixelSize logical, unsigned scale_120ths) noexcept
{
  return {ToPhysicalPixels(logical.width, scale_120ths),
          ToPhysicalPixels(logical.height, scale_120ths)};
}

[[gnu::const]]
constexpr int
ToPhysicalCoord(int logical_px, unsigned scale_120ths) noexcept
{
  if (logical_px < 0)
    return -(int)ToPhysicalPixels((unsigned)(-logical_px), scale_120ths);
  return (int)ToPhysicalPixels((unsigned)logical_px, scale_120ths);
}

[[gnu::const]]
constexpr unsigned
ToLogicalPixels(unsigned physical_px, unsigned scale_120ths) noexcept
{
  if (scale_120ths == 0)
    return physical_px;

  return (unsigned)(((std::uint64_t)physical_px * SCALE_100 +
                     scale_120ths / 2) / scale_120ths);
}

[[gnu::const]]
constexpr PixelSize
ToLogicalSize(PixelSize physical, unsigned scale_120ths) noexcept
{
  return {ToLogicalPixels(physical.width, scale_120ths),
          ToLogicalPixels(physical.height, scale_120ths)};
}

[[gnu::const]]
constexpr PixelPoint
ToPhysicalPoint(PixelPoint p, unsigned scale_120ths) noexcept
{
  return {ToPhysicalCoord(p.x, scale_120ths),
          ToPhysicalCoord(p.y, scale_120ths)};
}

/**
 * Integer wl_surface buffer_scale.  125% stays 1; 150% becomes 2.
 */
[[gnu::const]]
constexpr unsigned
ToIntegerScale(unsigned scale_120ths) noexcept
{
  const unsigned n = ToPhysicalPixels(1, scale_120ths);
  return n < 1 ? 1 : n;
}

[[gnu::const]]
constexpr unsigned
FromIntegerScale(unsigned n) noexcept
{
  return (n < 1 ? 1 : n) * SCALE_100;
}

/**
 * Convert hardware vs compositor-logical size into 120ths.
 * Example: 2880 / 1440 → 240 (200%).
 */
[[gnu::const]]
constexpr unsigned
ToScale120ths(unsigned physical_px, unsigned logical_px) noexcept
{
  if (logical_px == 0)
    return SCALE_100;

  return (unsigned)(((std::uint64_t)physical_px * SCALE_100 +
                     logical_px / 2) / logical_px);
}

struct Buffer {
  PixelSize size;
  unsigned buffer_scale;
};

/**
 * Fractional (viewport): buffer in physical pixels, buffer_scale = 1.
 * Integer: buffer = logical * N, buffer_scale = N (must divide evenly).
 * Never mix buffer_scale > 1 with a viewport.
 */
[[gnu::const]]
constexpr Buffer
ChooseBuffer(PixelSize logical, unsigned scale_120ths,
             bool fractional) noexcept
{
  if (fractional)
    return {ToPhysicalSize(logical, scale_120ths), 1};

  const unsigned n = ToIntegerScale(scale_120ths);
  return {{logical.width * n, logical.height * n}, n};
}

} // namespace Wayland
