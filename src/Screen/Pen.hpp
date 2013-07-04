/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
    DASH = PS_DASH,
    BLANK = PS_NULL
  };
#else
  enum Style {
    SOLID,
    DASH,
    BLANK
  };
#endif

protected:
#ifdef USE_GDI
  HPEN pen;
#else
  Color color;

  unsigned width;

#if defined(ENABLE_OPENGL) && !defined(HAVE_GLES)
  Style style;
#endif
#endif

public:
#ifdef USE_GDI

  /** Base Constructor for the Pen class */
  Pen() : pen(NULL) {}
  /**
   * Constructor that creates a Pen object, based on the given parameters
   * @param style Line style (SOLID, DASH, BLANK)
   * @param width Width of the line/Pen
   * @param c Color of the Pen
   */
  Pen(Style Style, unsigned width, const Color c):pen(NULL) {
    Set(Style, width, c);
  }
  /**
   * Constructor that creates a solid Pen object, based on the given parameters
   * @param width Width of the line/Pen
   * @param c Color of the Pen
   */
  Pen(unsigned width, Color c):pen(NULL) {
    Set(width, c);
  }

  /** Destructor */
  ~Pen() { Reset(); }

  Pen(const Pen &other) = delete;
  Pen &operator=(const Pen &other) = delete;

#else /* !USE_GDI */

  Pen():width(0) {}

  constexpr
  Pen(Style _style, unsigned _width, const Color _color)
    :color(_color), width(_width)
#if defined(ENABLE_OPENGL) && !defined(HAVE_GLES)
    , style(_style)
#endif
  {}

  constexpr
  Pen(unsigned _width, const Color _color)
    :color(_color), width(_width)
#if defined(ENABLE_OPENGL) && !defined(HAVE_GLES)
    , style(SOLID)
#endif
  {}

#endif /* !USE_GDI */

public:
  /**
   * Sets the Pens parameters to the given values
   * @param style Line style (SOLID, DASH, BLANK)
   * @param width Width of the line/Pen
   * @param c Color of the Pen
   */
  void Set(Style style, unsigned width, const Color c);
  /**
   * Sets the Pens parameters to the given values
   * @param width Width of the line/Pen
   * @param c Color of the Pen
   */
  void Set(unsigned width, const Color c);
  /**
   * Resets the Pen to NULL
   */
  void Reset();

  /**
   * Returns whether the Pen is defined (!= NULL)
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
  /**
   * Configures this pen in the OpenGL context.
   */
  void Set() const {
    color.Set();

#ifdef HAVE_GLES
    glLineWidthx(width << 16);
#else
    glLineWidth(width);
#endif
  }

  /**
   * Configure the Pen in the OpenGL context.  Don't forget to call
   * UnbindStyle() when you're done with this Pen.
   */
  void Bind() const {
    Set();

#ifndef HAVE_GLES
    if (style == DASH) {
      /* XXX implement for OpenGL/ES (using a 1D texture?) */
      glLineStipple(2, 0x1f1f);
      glEnable(GL_LINE_STIPPLE);
    }
#endif
  }

  void Unbind() const {
#ifndef HAVE_GLES
    if (style == DASH) {
      glDisable(GL_LINE_STIPPLE);
    }
#endif
  }
#endif /* OPENGL */
};

#ifndef USE_GDI

inline void
Pen::Reset()
{
  assert(!IsDefined() || IsScreenInitialized());

#ifndef NDEBUG
  width = 0;
#endif
}

#endif

#endif
