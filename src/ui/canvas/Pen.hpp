// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Features.hpp"
#include "Screen/Debug.hpp"

#include <cassert>

/**
 * A pen draws lines and borders.
 */
class Pen
{
public:
#ifdef USE_MEMORY_CANVAS
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
  Color color;

  uint8_t width = 0;

#if defined(USE_MEMORY_CANVAS) || defined(ENABLE_OPENGL)
  Style style;
#endif

public:
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
    return width > 0;
  }

  unsigned GetWidth() const noexcept {
    return width;
  }

  const Color GetColor() const noexcept {
    return color;
  }

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

inline void
Pen::Destroy() noexcept
{
  assert(!IsDefined() || IsScreenInitialized());

#ifndef NDEBUG
  width = 0;
#endif
}
