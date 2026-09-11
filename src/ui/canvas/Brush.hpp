// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Color.hpp"
#include "Screen/Debug.hpp"

#include <cassert>

/**
 * A Brush is used for drawing filled circles, rectangles and so on
 */
class Brush
{
protected:
  Color color = Color::Transparent();

public:
  Brush() noexcept = default;

  constexpr explicit Brush(const Color _color) noexcept
    :color(_color)  {}

public:
  /**
   * Sets the Color of the Brush
   * @param c The new Color
   */
  void Create(const Color c);

  /**
   * Resets the Brush to nullptr
   */
  void Destroy() noexcept;

  /**
   * Returns whether the Brush is defined (!= nullptr)
   * @return True if the Brush is defined, False otherwise
   */
  bool IsDefined() const noexcept {
    return !color.IsTransparent();
  }

  constexpr bool IsHollow() const noexcept {
    return color.IsTransparent();
  }

  const Color GetColor() const noexcept {
    return color;
  }

#ifdef ENABLE_OPENGL
  /**
   * Configures this brush in the OpenGL context.
   */
  void Bind() const noexcept {
    color.Bind();
  }

  void BindUniform(GLint location) const noexcept {
    color.Uniform(location);
  }
#endif /* OPENGL */
};

inline void
Brush::Create(const Color c)
{
  assert(IsScreenInitialized());

  color = c;
}

inline void
Brush::Destroy() noexcept
{
  assert(!IsDefined() || IsScreenInitialized());

  color = Color::Transparent();
}
