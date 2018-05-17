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

#ifndef XCSOAR_SCREEN_MEMORY_COLOR_HPP
#define XCSOAR_SCREEN_MEMORY_COLOR_HPP

#include "Screen/PortableColor.hpp"
#include "Compiler.h"

#include <stdint.h>


/* Workaround: Some Win32 headers define OPAQUE and TRANSPARENT as preprocessor
 * defines. Undefine them to avoid name conflict. */
#ifdef OPAQUE
#undef OPAQUE
#endif

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif


/**
 * This class represents a color in the RGB color space.  This is used
 * for compile-time constant colors, or for colors loaded from the
 * configuration.
 */
class Color {
#ifdef GREYSCALE
  Luminosity8 luminosity;
  uint8_t alpha;
#else
  BGRA8Color value;
#endif

public:
  static constexpr uint8_t TRANSPARENT = 0x00;
  static constexpr uint8_t OPAQUE = 0xff;

  Color() = default;

#ifdef GREYSCALE
  explicit constexpr Color(uint8_t _luminosity,
                           uint8_t _alpha=OPAQUE)
    :luminosity(_luminosity), alpha(_alpha) {}

  constexpr Color(uint8_t r, uint8_t g, uint8_t b,
                  uint8_t _alpha=OPAQUE)
    :luminosity(r, g, b), alpha(_alpha) {}

  explicit constexpr Color(RGB8Color other, uint8_t _alpha=OPAQUE)
    :luminosity(other), alpha(_alpha) {}

#else
  constexpr
  Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a=OPAQUE)
    :value({r, g, b, a}) {}

  explicit constexpr Color(RGB8Color other)
    :value({other.Red(), other.Green(), other.Blue(), OPAQUE}) {}
#endif

#ifdef GREYSCALE
  constexpr uint8_t GetLuminosity() const {
    return luminosity.GetLuminosity();
  }
#else
  /**
   * Returns the red part of the color
   * @return The red part of the color (0-255)
   */
  constexpr
  uint8_t Red() const
  {
    return value.Red();
  }

  /**
   * Returns the green part of the color
   * @return The green part of the color (0-255)
   */
  constexpr
  uint8_t Green() const
  {
    return value.Green();
  }

  /**
   * Returns the blue part of the color
   * @return The blue part of the color (0-255)
   */
  constexpr
  uint8_t Blue() const
  {
    return value.Blue();
  }
#endif

  /**
   * Returns the alpha part of the color
   * @return The alpha part of the color (0-255)
   */
  constexpr
  uint8_t
  Alpha() const
  {
#ifdef GREYSCALE
    return alpha;
#else
    return value.Alpha();
#endif
  }

  constexpr
  Color
  WithAlpha(uint8_t alpha) const {
#ifdef GREYSCALE
    return Color(GetLuminosity(), alpha);
#else
    return Color(Red(), Green(), Blue(), alpha);
#endif
  }

  constexpr bool IsOpaque() const {
    return Alpha() == OPAQUE;
  }

  constexpr bool IsTransparent() const {
    return Alpha() == TRANSPARENT;
  }

  /**
   * Construct a #Color object that is transparent.
   */
  static constexpr Color Transparent() {
    return Color(0, 0, 0, TRANSPARENT);
  }

  /**
   * Returns the highlighted version of this color.
   */
  constexpr
  Color
  Highlight() const
  {
#ifdef GREYSCALE
    return Color((GetLuminosity() + 0xff * 3) / 4);
#else
    return Color((Red() + 0xff * 3) / 4,
                 (Green() + 0xff * 3) / 4,
                 (Blue() + 0xff * 3) / 4);
#endif
  }

  /**
   * Returns the shadowed version of this color.
   */
  constexpr Color Shadow() const {
#ifdef GREYSCALE
    return Color(GetLuminosity() * 15u / 16u);
#else
    return Color(Red() * 15u / 16u,
                 Green() * 15u / 16u,
                 Blue() * 15u / 16u,
                 Alpha());
#endif
  }

  /**
   * Compares two colors
   * @param a Color 1
   * @param b Color 2
   * @return True if colors match, False otherwise
   */
  constexpr
  bool operator ==(const Color other) const
  {
    // TODO: compare alpha?
#ifdef GREYSCALE
    return GetLuminosity() == other.GetLuminosity();
#else
    return value.Red() == other.value.Red() &&
      value.Green() == other.value.Green() &&
      value.Blue() == other.value.Blue();
#endif
  }

  /**
   * Compares two colors (negative)
   * @param a Color 1
   * @param b Color 2
   * @return True if color do not match, False otherwise
   */
  constexpr
  bool operator !=(const Color other) const
  {
    return !(*this == other);
  }
};

#endif
