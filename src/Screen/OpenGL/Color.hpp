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

#ifndef XCSOAR_SCREEN_OPENGL_COLOR_HPP
#define XCSOAR_SCREEN_OPENGL_COLOR_HPP

#ifdef ANDROID
#include <GLES/gl.h>
#else
#include <SDL/SDL_opengl.h>
#endif

#ifndef ANDROID
#include <SDL_video.h> // for SDL_Color
#endif

/**
 * This class represents a color in the RGB color space.  This is used
 * for compile-time constant colors, or for colors loaded from the
 * configuration.
 */
struct Color {
#ifdef ANDROID
  GLfixed r, g, b, a;

  Color(GLubyte _r, GLubyte _g, GLubyte _b)
    :r(_r << 8u), g(_g << 8u), b(_b << 8u), a(1u << 16u) {}
  Color(GLubyte _r, GLubyte _g, GLubyte _b, GLubyte _a)
    :r(_r << 8u), g(_g << 8u), b(_b << 8u), a(_a << 8u) {}
#else
  GLubyte r, g, b, a;

  Color(GLubyte _r, GLubyte _g, GLubyte _b):r(_r), g(_g), b(_b), a(255) {}
  Color(GLubyte _r, GLubyte _g, GLubyte _b, GLubyte _a)
    :r(_r), g(_g), b(_b), a(_a) {}
#endif

  Color() {}

  /**
   * Returns the red part of the color
   * @return The red part of the color (0-255)
   */
  uint8_t
  red() const
  {
#ifdef ANDROID
    return (uint8_t)(r >> 8u);
#else
    return r;
#endif
  }

  /**
   * Returns the green part of the color
   * @return The green part of the color (0-255)
   */
  uint8_t
  green() const
  {
#ifdef ANDROID
    return (uint8_t)(g >> 8u);
#else
    return g;
#endif
  }

  /**
   * Returns the blue part of the color
   * @return The blue part of the color (0-255)
   */
  uint8_t
  blue() const
  {
#ifdef ANDROID
    return (uint8_t)(b >> 8u);
#else
    return b;
#endif
  }

#ifndef ANDROID
  /**
   * Convert this object to a SDL_Color.
   *
   * This function must be removed as soon as we have a pure OpenGL
   * renderer (without libSDL).
   */
  operator const SDL_Color() const {
    SDL_Color color;
    color.r = red();
    color.g = green();
    color.b = blue();
    color.unused = SDL_ALPHA_OPAQUE;
    return color;
  }
#endif

  Color
  with_alpha(GLubyte alpha) const {
    return Color(red(), green(), blue(), alpha);
  }

  /**
   * Returns the highlighted version of this color.
   */
  Color
  highlight() const
  {
#ifdef ANDROID
    return Color((r + 3) / 4., (g + 3) / 4., (b + 3) / 4.);
#else
    return Color((r + 0xff * 3) / 4, (g + 0xff * 3) / 4, (b + 0xff * 3) / 4);
#endif
  }

  /**
   * Configures this color in the OpenGL context.
   */
  void set() const {
#ifdef ANDROID
    /* on Android, glColor4ub() is not implemented, and we're forced
       to use floating point math for something as trivial as
       configuring a RGB color value */
    glColor4x(r, g, b, a);
#else
    glColor4ub(r, g, b, a);
#endif
  }

  /**
   * Compares two colors
   * @param a Color 1
   * @param b Color 2
   * @return True if colors match, False otherwise
   */
  bool operator ==(const Color &other) const
  {
    return r == other.r
      && g == other.g
      && b == other.b;
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

#endif
