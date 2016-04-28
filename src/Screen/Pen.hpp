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

#ifndef XCSOAR_SCREEN_PEN_HPP
#define XCSOAR_SCREEN_PEN_HPP

#include "Screen/Color.hpp"
#include "Screen/Features.hpp"
#include "Debug.hpp"

#include <assert.h>

/**
 * A pen draws lines and borders.
 */
class Pen
{
public:
#ifdef USE_GDI
  enum Style {
    SOLID = PS_SOLID,
    DASH1 = PS_DASH,
    DASH2 = PS_DASH,
    DASH3 = PS_DASH,
    BLANK = PS_NULL
  };
#elif defined(USE_MEMORY_CANVAS)
  typedef uint8_t Style;
  static constexpr uint8_t SOLID = -1;
  static constexpr uint8_t DASH1= -1-0b1000;
  static constexpr uint8_t DASH2= -1-0b1000;
  static constexpr uint8_t DASH3= -1-0b1000;
  static constexpr uint8_t BLANK = 0;
#else
  enum Style : uint8_t {
    SOLID,
    DASH1,
    DASH2,
    DASH3,
    BLANK
  };
#endif

protected:
#ifdef USE_GDI
  HPEN pen = nullptr;
#else
  Color color;

  uint8_t width = 0;

#if defined(USE_MEMORY_CANVAS) || (defined(ENABLE_OPENGL) && !defined(HAVE_GLES))
  Style style;
#endif
#endif

public:
#ifdef USE_GDI

  /** Base Constructor for the Pen class */
  Pen() = default;

  /**
   * Constructor that creates a Pen object, based on the given parameters
   * @param style Line style (SOLID, DASH1/2/3, BLANK)
   * @param width Width of the line/Pen
   * @param c Color of the Pen
   */
  Pen(Style Style, unsigned width, const Color c) {
    Create(Style, width, c);
  }

  /**
   * Constructor that creates a solid Pen object, based on the given parameters
   * @param width Width of the line/Pen
   * @param c Color of the Pen
   */
  Pen(unsigned width, Color c) {
    Create(width, c);
  }

  /** Destructor */
  ~Pen() {
    Destroy();
  }

  Pen(const Pen &other) = delete;
  Pen &operator=(const Pen &other) = delete;

#else /* !USE_GDI */

  Pen() = default;

  constexpr
  Pen(Style _style, unsigned _width, const Color _color)
    :color(_color), width(_width)
#if defined(USE_MEMORY_CANVAS) || (defined(ENABLE_OPENGL) && !defined(HAVE_GLES))
    , style(_style)
#endif
  {}

  constexpr
  Pen(unsigned _width, const Color _color)
    :color(_color), width(_width)
#if defined(USE_MEMORY_CANVAS) || (defined(ENABLE_OPENGL) && !defined(HAVE_GLES))
    , style(SOLID)
#endif
  {}

#endif /* !USE_GDI */

public:
  /**
   * Sets the Pens parameters to the given values
   * @param style Line style (SOLID, DASH1/2/3, BLANK)
   * @param width Width of the line/Pen
   * @param c Color of the Pen
   */
  void Create(Style style, unsigned width, const Color c);

  /**
   * Sets the Pens parameters to the given values
   * @param width Width of the line/Pen
   * @param c Color of the Pen
   */
  void Create(unsigned width, const Color c);

  /**
   * Resets the Pen to nullptr
   */
  void Destroy();

  /**
   * Returns whether the Pen is defined (!= nullptr)
   * @return True if the Pen is defined, False otherwise
   */
  bool
  IsDefined() const
  {
#ifdef USE_GDI
    return pen != nullptr;
#else
    return width > 0;
#endif
  }

#ifdef USE_GDI
  /**
   * Returns the native HPEN object
   * @return The native HPEN object
   */
  HPEN Native() const { return pen; }
#else
  unsigned
  GetWidth() const
  {
    return width;
  }

  const Color
  GetColor() const
  {
    return color;
  }
#endif

#ifdef ENABLE_OPENGL
private:
  void BindStyle() const {
#if defined(HAVE_GLES) && !defined(HAVE_GLES2)
    glLineWidthx(width << 16);
#else
    glLineWidth(width);
#endif

#ifndef HAVE_GLES
    if (style == DASH1) {
      /* XXX implement for OpenGL/ES (using a 1D texture?) */
      glLineStipple(2, 0x1818);
      glEnable(GL_LINE_STIPPLE);
    } else if (style == DASH2) {
      /* XXX implement for OpenGL/ES (using a 1D texture?) */
      glLineStipple(2, 0x1f1f);
      glEnable(GL_LINE_STIPPLE);
    } else if (style == DASH3) {
      /* XXX implement for OpenGL/ES (using a 1D texture?) */
      glLineStipple(2, 0x8f8f);
      glEnable(GL_LINE_STIPPLE);
    }
#endif
  }

public:
  /**
   * Configure the Pen in the OpenGL context.  Don't forget to call
   * Unbind() when you're done with this Pen.
   */
  void Bind() const {
    color.Bind();
    BindStyle();
  }

#ifdef USE_GLSL
  void BindUniform(GLint location) const {
    color.Uniform(location);
    BindStyle();
  }
#endif

  void Unbind() const {
#ifndef HAVE_GLES
    if ((style == DASH1) || (style == DASH2) || (style == DASH3)) {
      glDisable(GL_LINE_STIPPLE);
    }
#endif
  }
#endif /* OPENGL */

#ifdef USE_MEMORY_CANVAS
  constexpr unsigned GetMask() const {
    return style | (-1 & ~0xff);
  }
#endif
};

#ifndef USE_GDI

inline void
Pen::Destroy()
{
  assert(!IsDefined() || IsScreenInitialized());

#ifndef NDEBUG
  width = 0;
#endif
}

#endif

#endif
