// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * User-selected display technology.  Affects refresh-sensitive UI
 * (scrolling, animations) and font rasterisation (Kobo 1-bit glyphs
 * on e-paper).  Full greyscale / dither chrome stays compile-time
 * #DITHER so color e-ink keeps colour.
 */
enum class DisplayType : uint8_t {
  /** Conventional LCD or OLED; full animations. */
  LCD,

  /** Monochrome electronic paper (slow refresh). */
  E_INK,

  /** Color electronic paper (still slow refresh). */
  COLOR_E_INK,

  COUNT
};

[[gnu::const]]
constexpr bool
IsEPaperDisplayType(DisplayType type) noexcept
{
  return type == DisplayType::E_INK ||
    type == DisplayType::COLOR_E_INK;
}

/**
 * 1-bit alias glyphs like Kobo (no anti-alias).  E-paper panels
 * turn grey fringe into extra ink, which closes counters and reads as
 * over-bold strokes; color e-ink uses the same raster path.
 */
[[gnu::const]]
constexpr bool
DisplayTypeUsesMonochromeFonts(DisplayType type) noexcept
{
  return IsEPaperDisplayType(type);
}
