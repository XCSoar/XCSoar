/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_SDL_COLOR_HPP
#define XCSOAR_SCREEN_SDL_COLOR_HPP

#include <SDL_video.h>
#include <stdint.h>

/**
 * This class represents a color in the RGB color space.  This is used
 * for compile-time constant colors, or for colors loaded from the
 * configuration.
 */
struct Color {
  SDL_Color value;

  Color() {}

  Color(uint8_t r, uint8_t g, uint8_t b)
  {
    value.r = r;
    value.g = g;
    value.b = b;
    value.unused = SDL_ALPHA_OPAQUE; // alpha for SDL_gfx, see gfx_color()
  }

  /**
   * Returns the red part of the color
   * @return The red part of the color (0-255)
   */
  uint8_t red() const
  {
    return value.r;
  }

  /**
   * Returns the green part of the color
   * @return The green part of the color (0-255)
   */
  uint8_t green() const
  {
    return value.g;
  }

  /**
   * Returns the blue part of the color
   * @return The blue part of the color (0-255)
   */
  uint8_t blue() const
  {
    return value.b;
  }

  operator const SDL_Color() const {
    return value;
  }

  Uint32 gfx_color() const {
    return ((Uint32)value.r << 24) | ((Uint32)value.g << 16) |
      ((Uint32)value.b << 8) | (Uint32)value.unused;
  }

  /**
   * Returns the highlighted version of this color.
   */
  Color
  highlight() const
  {
    return Color((value.r + 0xff * 3) / 4,
                 (value.g + 0xff * 3) / 4,
                 (value.b + 0xff * 3) / 4);
  }

  /**
   * Compares two colors
   * @param a Color 1
   * @param b Color 2
   * @return True if colors match, False otherwise
   */
  bool operator ==(const Color &other) const
  {
    return value.r == other.value.r
      && value.g == other.value.g
      && value.b == other.value.b;
  }

  /**
   * Compares two colors (negative)
   * @param a Color 1
   * @param b Color 2
   * @return True if color do not match, False otherwise
   */
  bool operator !=(const Color &other) const
  {
    return !(*this == other);
  }
};

#ifndef ENABLE_OPENGL

/**
 * A hardware color on a specific Canvas.  A Canvas maps a Color
 * object into HWColor.  Depending on the platform, Color and
 * HWColor may be different, e.g. if the Canvas can not display 24
 * bit RGB colors.
 */
struct HWColor {
  Uint32 value;

  HWColor():value(0) {}
  explicit HWColor(Uint32 c):value(c) {}

  operator Uint32() const { return value; }
};

#endif /* !OPENGL */

#endif
