// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Scope.hpp"
#include "Globals.hpp"
#include "Canvas.hpp"
#include "ui/opengl/Features.hpp" // for SOFTWARE_ROTATE_DISPLAY

#include <cmath>

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "Rotate.hpp"

class GLCanvasScissor : public GLEnable<GL_SCISSOR_TEST> {
public:
  [[nodiscard]]
  explicit GLCanvasScissor(const Canvas &canvas) noexcept
    :GLCanvasScissor(canvas.GetRect()) {}

  [[nodiscard]]
  explicit GLCanvasScissor(PixelRect rc) noexcept {
    Scissor(rc);
  }

private:
  void Scissor(PixelRect rc) noexcept {
    OpenGL::ToViewport(rc);
    const auto p = OpenGL::ToPhysicalRect(rc.left, rc.top,
                                          rc.GetWidth(), rc.GetHeight());
    ::glScissor(p.x, p.y, p.width, p.height);
  }
};

#else

/**
 * When HiDPI uses logical coords for projection but glViewport is
 * physical, glScissor must be in physical coords.  Convert logical
 * PixelRect to (x, y, width, height) in physical viewport coords.
 * Needed for List, VScrollPanel, etc. when window_size != viewport_size.
 */
struct GLCanvasScissorParams {
  GLint x, y;
  GLsizei width, height;
};

[[gnu::const]]
static inline GLCanvasScissorParams
ToPhysicalScissor(PixelRect rc) noexcept
{
  const int x = OpenGL::translate.x + rc.left;
  const int y = OpenGL::viewport_size.y - OpenGL::translate.y - rc.bottom;
  int w = rc.GetWidth();
  int h = rc.GetHeight();

  if (OpenGL::window_size.x != OpenGL::viewport_size.x ||
      OpenGL::window_size.y != OpenGL::viewport_size.y) {
    const float sx = float(OpenGL::window_size.x) / OpenGL::viewport_size.x;
    const float sy = float(OpenGL::window_size.y) / OpenGL::viewport_size.y;
    const auto px = static_cast<GLint>(std::round(x * sx));
    const auto py = static_cast<GLint>(std::round(y * sy));
    w = static_cast<int>(std::round(w * sx));
    h = static_cast<int>(std::round(h * sy));
    /* glScissor rejects negative width/height */
    if (w < 0)
      w = 0;
    if (h < 0)
      h = 0;
    return {px, py, static_cast<GLsizei>(w), static_cast<GLsizei>(h)};
  }
  if (w < 0)
    w = 0;
  if (h < 0)
    h = 0;
  return {static_cast<GLint>(x), static_cast<GLint>(y),
          static_cast<GLsizei>(w), static_cast<GLsizei>(h)};
}

class GLCanvasScissor : public GLScissor {
public:
  [[nodiscard]]
  GLCanvasScissor(const Canvas &canvas) noexcept
    :GLCanvasScissor(PixelRect(0, 0, canvas.GetWidth(), canvas.GetHeight())) {}

  [[nodiscard]]
  explicit GLCanvasScissor(PixelRect rc) noexcept
    :GLCanvasScissor(ToPhysicalScissor(rc)) {}

  [[nodiscard]]
  GLCanvasScissor(GLCanvasScissorParams p) noexcept
    :GLScissor(p.x, p.y, p.width, p.height) {}
};

#endif
