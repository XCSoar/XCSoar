// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Scope.hpp"
#include "Globals.hpp"
#include "Canvas.hpp"
#include "ui/opengl/Features.hpp" // for SOFTWARE_ROTATE_DISPLAY

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
 * PixelRect to (x, y, width, height) in physical window coords.
 * Needed for List, VScrollPanel, etc. when window_size != viewport_size.
 */
[[gnu::const]]
static inline OpenGL::PhysicalRect
ToPhysicalScissor(PixelRect rc) noexcept
{
  const int x = OpenGL::translate.x + rc.left;
  const int y = OpenGL::viewport_size.y - OpenGL::translate.y - rc.bottom;
  const int w = rc.GetWidth();
  const int h = rc.GetHeight();
  return OpenGL::ToPhysicalRect(x, y, w, h);
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
  GLCanvasScissor(OpenGL::PhysicalRect p) noexcept
    :GLScissor(p.x, p.y, p.width, p.height) {}
};

#endif
