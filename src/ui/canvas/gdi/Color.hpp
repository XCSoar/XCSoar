// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/PortableColor.hpp"

#include <cstdint>

#include <windef.h>
#include <wingdi.h>

/**
 * This class represents a color in the RGB color space.  This is used
 * for compile-time constant colors, or for colors loaded from the
 * configuration.
 */
class Color {
  COLORREF value;

public:
  /** Base Constructor (creates an undefined Color object) */
  Color() = default;

  /**
   * Constructor (creates a Color object based on the given COLORREF)
   * @param c COLORREF (e.g. 0xFF6677)
   */
  explicit constexpr Color(COLORREF c) : value(c) {}
  /**
   * Constructor (creates a Color object based on the given color parts)
   * @param r Red part
   * @param g Green part
   * @param b Blue part
   */
  constexpr Color(uint8_t r, uint8_t g, uint8_t b) : value(RGB(r, g, b)) {}

  explicit constexpr Color(RGB8Color other)
    :value(RGB(other.Red(), other.Green(), other.Blue())) {}

  explicit constexpr Color(BGRA8Color src) noexcept
    :value(RGB(src.Red(), src.Green(), src.Blue())) {}

  /**
   * Returns the red part of the color
   * @return The red part of the color (0-255)
   */
  constexpr
  uint8_t Red() const
  {
    return GetRValue(value);
  }

  /**
   * Returns the green part of the color
   * @return The green part of the color (0-255)
   */
  constexpr
  uint8_t Green() const
  {
    return GetGValue(value);
  }

  /**
   * Returns the blue part of the color
   * @return The blue part of the color (0-255)
   */
  constexpr
  uint8_t Blue() const
  {
    return GetBValue(value);
  }

  Color
  &operator =(COLORREF c)
  {
    value = c;
    return *this;
  }

  constexpr
  operator COLORREF() const { return value; }

  constexpr COLORREF GetNative() const {
    return value;
  }

  /**
   * Returns the highlighted version of this color.
   */
  constexpr
  Color
  Highlight() const
  {
    return Color((Red() + 0xff * 3) / 4,
                 (Green() + 0xff * 3) / 4,
                 (Blue() + 0xff * 3) / 4);
  }

  /**
   * Returns the shadowed version of this color.
   */
  constexpr Color Shadow() const {
    return Color(Red() * 15u / 16u,
                 Green() * 15u / 16u,
                 Blue() * 15u / 16u);
  }
};

/**
 * Compares two colors
 * @param a Color 1
 * @param b Color 2
 * @return True if colors match, False otherwise
 */
constexpr bool
operator ==(const Color a, const Color b)
{
  return a.GetNative() == b.GetNative();
}

/**
 * Compares two colors (negative)
 * @param a Color 1
 * @param b Color 2
 * @return True if color do not match, False otherwise
 */
constexpr bool
operator !=(const Color a, const Color b)
{
  return !(a == b);
}

/**
 * A hardware color on a specific Canvas.  A Canvas maps a Color
 * object into HWColor.  Depending on the platform, Color and
 * HWColor may be different, e.g. if the Canvas can not display 24
 * bit RGB colors.
 */
class HWColor {
  COLORREF value;

public:
  constexpr HWColor():value(0) {}
  explicit constexpr HWColor(COLORREF c):value(c) {}

  constexpr operator COLORREF() const { return value; }
};
