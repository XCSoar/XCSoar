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
    ApplyPixelScale(rc);
    ::glScissor(rc.left, rc.top, rc.GetWidth(), rc.GetHeight());
  }

  static void ApplyPixelScale(PixelRect &rc) noexcept {
    const unsigned scale = OpenGL::viewport_pixel_scale;
    if (scale <= 1)
      return;

    rc.left *= scale;
    rc.top *= scale;
    rc.right *= scale;
    rc.bottom *= scale;
  }
};

#else

class GLCanvasScissor : public GLScissor {
public:
  [[nodiscard]]
  GLCanvasScissor(const Canvas &canvas) noexcept
    :GLScissor(ScissorX(OpenGL::translate.x),
               ScissorY(OpenGL::viewport_size.y - OpenGL::translate.y -
                        canvas.GetHeight()),
               ScissorSize(canvas.GetWidth()), ScissorSize(canvas.GetHeight())) {}

  [[nodiscard]]
  explicit GLCanvasScissor(PixelRect rc) noexcept
    :GLScissor(ScissorX(OpenGL::translate.x + rc.left),
               ScissorY(OpenGL::viewport_size.y - OpenGL::translate.y -
                        rc.bottom),
               ScissorSize(rc.GetWidth()), ScissorSize(rc.GetHeight())) {}

private:
  [[gnu::const]]
  static GLint ScissorX(GLint x) noexcept {
    return x * GLint(OpenGL::viewport_pixel_scale);
  }

  [[gnu::const]]
  static GLint ScissorY(GLint y) noexcept {
    return y * GLint(OpenGL::viewport_pixel_scale);
  }

  [[gnu::const]]
  static GLsizei ScissorSize(unsigned size) noexcept {
    return GLsizei(size * OpenGL::viewport_pixel_scale);
  }
};

#endif
