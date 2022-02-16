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

#ifndef XCSOAR_SCREEN_PEN_HPP
#define XCSOAR_SCREEN_PEN_HPP

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Features.hpp"
#include "Screen/Debug.hpp"

#include <cassert>

#ifdef USE_GDI
#include <wingdi.h>
#endif

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
  };
#elif defined(USE_MEMORY_CANVAS)
  enum Style : uint8_t {
    SOLID = uint8_t(~0),
    DASH1 = uint8_t(~0 - 0b1000),
    DASH2 = uint8_t(~0 - 0b1000),
    DASH3 = uint8_t(~0 - 0b1000),
  };
#else
  enum Style : uint8_t {
    SOLID,
    DASH1,
    DASH2,
    DASH3,
  };
#endif

protected:
#ifdef USE_GDI
  HPEN pen = nullptr;
#else
  Color color;

  uint8_t width = 0;

#if defined(USE_MEMORY_CANVAS) || defined(ENABLE_OPENGL)
  Style style;
#endif
#endif

public:
#ifdef USE_GDI

  /** Base Constructor for the Pen class */
  Pen() noexcept = default;

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
  ~Pen() noexcept {
    Destroy();
  }

  Pen(const Pen &other) = delete;
  Pen &operator=(const Pen &other) = delete;

#else /* !USE_GDI */

  Pen() noexcept = default;

  constexpr Pen(Style _style, unsigned _width, const Color _color) noexcept
    :color(_color), width(_width)
#if defined(USE_MEMORY_CANVAS) || defined(ENABLE_OPENGL)
    , style(_style)
#endif
  {}

  constexpr Pen(unsigned _width, const Color _color) noexcept
    :color(_color), width(_width)
#if defined(USE_MEMORY_CANVAS) || defined(ENABLE_OPENGL)
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
  void Destroy() noexcept;

  /**
   * Returns whether the Pen is defined (!= nullptr)
   * @return True if the Pen is defined, False otherwise
   */
  bool IsDefined() const noexcept {
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
  HPEN Native() const noexcept { return pen; }
#else
  unsigned GetWidth() const noexcept {
    return width;
  }

  const Color GetColor() const noexcept {
    return color;
  }
#endif

#ifdef ENABLE_OPENGL
  Style GetStyle() const noexcept {
    return style;
  }

private:
  void BindStyle() const noexcept {
    glLineWidth(width);

    /* note: this ignores the "style" field; this needs to be done
       separately, with the "dashed_shader" */
  }

public:
  /**
   * Configure the Pen in the OpenGL context.  Don't forget to call
   * Unbind() when you're done with this Pen.
   */
  void Bind() const noexcept {
    color.Bind();
    BindStyle();
  }

  void BindUniform(GLint location) const noexcept {
    color.Uniform(location);
    BindStyle();
  }

  void Unbind() const noexcept {
  }
#endif /* OPENGL */

#ifdef USE_MEMORY_CANVAS
  constexpr unsigned GetMask() const noexcept {
    return style | (-1 & ~0xff);
  }
#endif
};

#ifndef USE_GDI

inline void
Pen::Destroy() noexcept
{
  assert(!IsDefined() || IsScreenInitialized());

#ifndef NDEBUG
  width = 0;
#endif
}

#endif

#endif
