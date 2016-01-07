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

#ifndef XCSOAR_SCREEN_OPENGL_COLOR_HPP
#define XCSOAR_SCREEN_OPENGL_COLOR_HPP

#include "Screen/PortableColor.hpp"
#include "Features.hpp"
#include "System.hpp"

#ifdef USE_GLSL
#include "Attribute.hpp"
#endif

#include <stdint.h>

/**
 * This class represents a color in the RGB color space.  This is used
 * for compile-time constant colors, or for colors loaded from the
 * configuration.
 */
class Color {
#ifdef HAVE_GLES
  typedef GLfixed Component;

  static constexpr Component Import(uint8_t value) {
    return value << 8;
  }

  static constexpr uint8_t Export(Component value) {
    return value >> 8;
  }

  static constexpr Component MAX = 1u << 16u;
#else
  typedef GLubyte Component;

  static constexpr Component Import(uint8_t value) {
    return value;
  }

  static constexpr uint8_t Export(Component value) {
    return value;
  }

  static constexpr Component MAX = 0xff;
#endif

  static constexpr GLfloat ExportFloat(Component value) {
    return GLfloat(value) / GLfloat(MAX);
  }

  Component r, g, b, a;

  struct Internal {};

  constexpr Color(Internal,
                  Component _r, Component _g, Component _b, Component _a)
    :r(_r), g(_g), b(_b), a(_a) {}

public:
  constexpr Color(uint8_t _r, uint8_t _g, uint8_t _b)
    :r(Import(_r)), g(Import(_g)), b(Import(_b)), a(MAX) {}
  constexpr Color(GLubyte _r, GLubyte _g, GLubyte _b, GLubyte _a)
    :r(Import(_r)), g(Import(_g)), b(Import(_b)), a(Import(_a)) {}

  explicit constexpr Color(RGB8Color other)
    :r(Import(other.Red())), g(Import(other.Green())), b(Import(other.Blue())),
     a(MAX) {}

  Color() = default;

#ifdef HAVE_GLES
  static constexpr GLenum TYPE = GL_FIXED;
#else
  static constexpr GLenum TYPE = GL_UNSIGNED_BYTE;
#endif

  /**
   * Returns the red part of the color
   * @return The red part of the color (0-255)
   */
  constexpr
  uint8_t
  Red() const
  {
    return Export(r);
  }

  /**
   * Returns the green part of the color
   * @return The green part of the color (0-255)
   */
  constexpr
  uint8_t
  Green() const
  {
    return Export(g);
  }

  /**
   * Returns the blue part of the color
   * @return The blue part of the color (0-255)
   */
  constexpr
  uint8_t
  Blue() const
  {
    return Export(b);
  }

  /**
   * Returns the alpha part of the color
   * @return The alpha part of the color (0-255)
   */
  constexpr
  uint8_t
  Alpha() const
  {
    return Export(a);
  }

  constexpr
  Color
  WithAlpha(uint8_t alpha) const {
    return Color(Internal(), r, g, b, Import(alpha));
  }

  constexpr bool IsOpaque() const {
#ifdef HAVE_GLES
    return a >= 0xff00;
#else
    return a == 0xff;
#endif
  }

  constexpr bool IsTransparent() const {
    return a == 0;
  }

  /**
   * Construct a #Color object that is transparent.
   */
  static constexpr Color Transparent() {
    return Color(0, 0, 0, 0);
  }

  /**
   * Returns the highlighted version of this color.
   */
  constexpr
  Color
  Highlight() const
  {
#ifdef HAVE_GLES
    return Color((r + 3) / 4., (g + 3) / 4., (b + 3) / 4.);
#else
    return Color((r + 0xff * 3) / 4, (g + 0xff * 3) / 4, (b + 0xff * 3) / 4);
#endif
  }

  /**
   * Returns the shadowed version of this color.
   */
  constexpr Color Shadow() const {
    return Color(Red() * 15u / 16u,
                 Green() * 15u / 16u,
                 Blue() * 15u / 16u,
                 Alpha());
  }

#ifdef USE_GLSL
  void Uniform(GLint location) const {
    glUniform4f(location, ExportFloat(r), ExportFloat(g),
                ExportFloat(b), ExportFloat(a));
  }

  void VertexAttrib(GLint index) const {
    glVertexAttrib4f(index, ExportFloat(r), ExportFloat(g),
                     ExportFloat(b), ExportFloat(a));
  }
#endif

  /**
   * Configures this color in the OpenGL context.
   */
  void Bind() const {
#ifdef USE_GLSL
    VertexAttrib(OpenGL::Attribute::COLOR);
#elif defined(HAVE_GLES)
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
  constexpr
  bool operator ==(const Color other) const
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
  constexpr
  bool operator !=(const Color other) const
  {
    return !(*this == other);
  }
};

struct ScopeColorPointer {
  ScopeColorPointer(const Color *p) {
#ifdef USE_GLSL
    glEnableVertexAttribArray(OpenGL::Attribute::COLOR);
    glVertexAttribPointer(OpenGL::Attribute::COLOR, 4, Color::TYPE,
                          GL_FALSE, 0, p);
#else
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, Color::TYPE, 0, p);
#endif
  }

  ~ScopeColorPointer() {
#ifdef USE_GLSL
    glDisableVertexAttribArray(OpenGL::Attribute::COLOR);
#else
    glDisableClientState(GL_COLOR_ARRAY);
#endif
  }
};

#endif
