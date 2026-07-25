// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * User-selected display technology.  Affects refresh-sensitive UI
 * (scrolling, animations).  Visual greyscale / dither for e-ink is
 * separate and may follow later.
 */
enum class DisplayType : uint8_t {
  /** Conventional LCD / OLED — full animations. */
  LCD,

  /** Monochrome electronic paper (slow refresh). */
  E_INK,

  /** Colour electronic paper (still slow refresh). */
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
