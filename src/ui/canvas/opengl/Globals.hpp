// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * A collection of global variables for the OpenGL backend.  Global
 * variables are not good style, but since there can be only once
 * OpenGL context at a time, this is good enough for XCSoar.
 *
 */

#pragma once

#include "ui/opengl/Features.hpp"
#include "ui/opengl/System.hpp"
#include "Math/Point2D.hpp"

#include <glm/fwd.hpp>
#include <cmath>

#ifdef SOFTWARE_ROTATE_DISPLAY
#include <cstdint>
enum class DisplayOrientation : uint8_t;
#endif

struct PixelPoint;

namespace OpenGL {

/**
 * Is the extension ARB_texture_non_power_of_two present?  If yes,
 * then textures can have any size, not just power of two.
 */
extern bool texture_non_power_of_two;

/**
 * Is glMapBuffer() available?  May be implemented by the extension
 * GL_OES_mapbuffer.
 */
extern bool mapbuffer;

/**
 * Which depth+stencil internalFormat is supported by the
 * Renderbuffer?
 */
extern GLenum render_buffer_depth_stencil;

/**
 * Which stencil internalFormat is supported by the Renderbuffer?
 */
extern GLenum render_buffer_stencil;

/**
 * The dimensions of the OpenGL window in pixels.
 */
extern UnsignedPoint2D window_size;

/**
 * The dimensions of the OpenGL viewport in pixels.
 */
extern UnsignedPoint2D viewport_size;

#ifdef SOFTWARE_ROTATE_DISPLAY
extern DisplayOrientation display_orientation;
#endif

/**
 * The current SubCanvas translation in pixels.
 */
extern PixelPoint translate;

extern glm::mat4 projection_matrix;

/**
 * Result of converting logical viewport (x,y,w,h) to physical window
 * coords for glScissor, glCopyTexSubImage2D, etc.
 */
struct PhysicalRect {
  GLint x, y;
  GLsizei width, height;
};

/**
 * Scale logical viewport (x,y,w,h) to physical window coords when
 * HiDPI or fractional scaling (window_size != viewport_size).  Use
 * for glScissor, glCopyTexSubImage2D and other GL APIs that take
 * window (physical) coordinates.
 */
[[gnu::const]]
inline PhysicalRect
ToPhysicalRect(int x, int y, int w, int h) noexcept
{
  if (window_size.x != viewport_size.x ||
      window_size.y != viewport_size.y) {
    const float sx = float(window_size.x) / viewport_size.x;
    const float sy = float(window_size.y) / viewport_size.y;
    auto pw = static_cast<int>(std::round(w * sx));
    auto ph = static_cast<int>(std::round(h * sy));
    if (pw < 0)
      pw = 0;
    if (ph < 0)
      ph = 0;
    return {static_cast<GLint>(std::round(x * sx)),
            static_cast<GLint>(std::round(y * sy)),
            static_cast<GLsizei>(pw), static_cast<GLsizei>(ph)};
  }
  if (w < 0)
    w = 0;
  if (h < 0)
    h = 0;
  return {static_cast<GLint>(x), static_cast<GLint>(y),
          static_cast<GLsizei>(w), static_cast<GLsizei>(h)};
}

} // namespace OpenGL
