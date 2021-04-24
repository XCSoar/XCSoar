/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ui/canvas/PortableColor.hpp"
#include "ui/opengl/Features.hpp"
#include "ui/opengl/System.hpp"
#include "Attribute.hpp"

#include <cstdint>

/**
 * This class represents a color in the RGB color space.  This is used
 * for compile-time constant colors, or for colors loaded from the
 * configuration.
 */
class Color {
#ifdef HAVE_GLES
  typedef GLfixed Component;

  static constexpr Component Import(uint8_t value) noexcept {
    return value << 8;
  }

  static constexpr uint8_t Export(Component value) noexcept {
    return value >> 8;
  }

  static constexpr Component MAX = 1u << 16u;
#else
  typedef GLubyte Component;

  static constexpr Component Import(uint8_t value) noexcept {
    return value;
  }

  static constexpr uint8_t Export(Component value) noexcept {
    return value;
  }

  static constexpr Component MAX = 0xff;
#endif

  static constexpr GLfloat ExportFloat(Component value) noexcept {
    return GLfloat(value) / GLfloat(MAX);
  }

  Component r, g, b, a;

  struct Internal {};

  constexpr Color(Internal,
                  Component _r, Component _g,
                  Component _b, Component _a) noexcept
    :r(_r), g(_g), b(_b), a(_a) {}

public:
  constexpr Color(uint8_t _r, uint8_t _g, uint8_t _b) noexcept
    :r(Import(_r)), g(Import(_g)), b(Import(_b)), a(MAX) {}
  constexpr Color(GLubyte _r, GLubyte _g, GLubyte _b, GLubyte _a) noexcept
    :r(Import(_r)), g(Import(_g)), b(Import(_b)), a(Import(_a)) {}

  explicit constexpr Color(RGB8Color other)
    :r(Import(other.Red())), g(Import(other.Green())), b(Import(other.Blue())),
     a(MAX) {}

  Color() noexcept = default;

#ifdef HAVE_GLES
  static constexpr GLenum TYPE = GL_FIXED;
#else
  static constexpr GLenum TYPE = GL_UNSIGNED_BYTE;
#endif

  /**
   * Returns the red part of the color
   * @return The red part of the color (0-255)
   */
  constexpr uint8_t Red() const noexcept {
    return Export(r);
  }

  /**
   * Returns the green part of the color
   * @return The green part of the color (0-255)
   */
  constexpr uint8_t Green() const noexcept {
    return Export(g);
  }

  /**
   * Returns the blue part of the color
   * @return The blue part of the color (0-255)
   */
  constexpr uint8_t Blue() const noexcept {
    return Export(b);
  }

  /**
   * Returns the alpha part of the color
   * @return The alpha part of the color (0-255)
   */
  constexpr uint8_t Alpha() const noexcept {
    return Export(a);
  }

  constexpr Color WithAlpha(uint8_t alpha) const noexcept {
    return Color(Internal(), r, g, b, Import(alpha));
  }

  constexpr bool IsOpaque() const noexcept {
#ifdef HAVE_GLES
    return a >= 0xff00;
#else
    return a == 0xff;
#endif
  }

  constexpr bool IsTransparent() const noexcept {
    return a == 0;
  }

  /**
   * Construct a #Color object that is transparent.
   */
  static constexpr Color Transparent() noexcept {
    return Color(0, 0, 0, 0);
  }

  /**
   * Returns the highlighted version of this color.
   */
  constexpr Color Highlight() const noexcept {
#ifdef HAVE_GLES
    return Color((r + 3) / 4., (g + 3) / 4., (b + 3) / 4.);
#else
    return Color((r + 0xff * 3) / 4, (g + 0xff * 3) / 4, (b + 0xff * 3) / 4);
#endif
  }

  /**
   * Returns the shadowed version of this color.
   */
  constexpr Color Shadow() const noexcept {
    return Color(Red() * 15u / 16u,
                 Green() * 15u / 16u,
                 Blue() * 15u / 16u,
                 Alpha());
  }

  void Uniform(GLint location) const noexcept {
    glUniform4f(location, ExportFloat(r), ExportFloat(g),
                ExportFloat(b), ExportFloat(a));
  }

  void VertexAttrib(GLint index) const noexcept {
    glVertexAttrib4f(index, ExportFloat(r), ExportFloat(g),
                     ExportFloat(b), ExportFloat(a));
  }

  /**
   * Configures this color in the OpenGL context.
   */
  void Bind() const noexcept {
    VertexAttrib(OpenGL::Attribute::COLOR);
  }

  /**
   * Compares two colors
   * @param a Color 1
   * @param b Color 2
   * @return True if colors match, False otherwise
   */
  constexpr bool operator ==(const Color other) const noexcept {
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
  constexpr bool operator !=(const Color other) const noexcept {
    return !(*this == other);
  }
};

struct ScopeColorPointer {
  ScopeColorPointer(const Color *p) noexcept {
    glEnableVertexAttribArray(OpenGL::Attribute::COLOR);
    glVertexAttribPointer(OpenGL::Attribute::COLOR, 4, Color::TYPE,
                          GL_FALSE, 0, p);
  }

  ~ScopeColorPointer() noexcept {
    glDisableVertexAttribArray(OpenGL::Attribute::COLOR);
  }
};

#endif
