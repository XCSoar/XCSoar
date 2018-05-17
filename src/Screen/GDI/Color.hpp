/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_SCREEN_GDI_COLOR_HPP
#define XCSOAR_SCREEN_GDI_COLOR_HPP

#include "Screen/PortableColor.hpp"

#include <windows.h>
#include <stdint.h>

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
static inline constexpr bool
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
static inline constexpr bool
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

#endif
