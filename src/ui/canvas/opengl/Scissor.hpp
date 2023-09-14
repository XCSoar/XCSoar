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
    ::glScissor(rc.left, rc.top, rc.GetWidth(), rc.GetHeight());
  }
};

#else

class GLCanvasScissor : public GLScissor {
public:
  [[nodiscard]]
  GLCanvasScissor(const Canvas &canvas) noexcept
    :GLScissor(OpenGL::translate.x,
               OpenGL::viewport_size.y - OpenGL::translate.y - canvas.GetHeight(),
               canvas.GetWidth(), canvas.GetHeight()) {}

  [[nodiscard]]
  explicit GLCanvasScissor(PixelRect rc) noexcept
    :GLScissor(OpenGL::translate.x + rc.left,
               OpenGL::viewport_size.y - OpenGL::translate.y - rc.bottom,
               rc.GetWidth(), rc.GetHeight()) {}
};

#endif
