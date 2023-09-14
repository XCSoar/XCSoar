// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/egl/System.hpp"

#include <optional>

namespace EGL {

class Display {
  EGLDisplay display = EGL_NO_DISPLAY;

  EGLConfig chosen_config;

  EGLContext context = EGL_NO_CONTEXT;

  /**
   * A 1x1 pbuffer surface that is used to activate the EGLContext
   * while we have no real surface.
   */
  EGLSurface dummy_surface = EGL_NO_SURFACE;

public:
  /**
   * Throws on error.
   */
  explicit Display(EGLNativeDisplayType native_display);

  ~Display() noexcept;

  /**
   * Throws on error.
   */
  EGLSurface CreateWindowSurface(EGLNativeWindowType native_window);

  /**
   * Throws on error.
   */
  void MakeCurrent(EGLSurface surface);

  void DestroySurface(EGLSurface surface) noexcept {
    eglDestroySurface(display, surface);
  }

  [[gnu::pure]]
  std::optional<EGLint> QuerySurface(EGLSurface surface, EGLint attribute) {
    int value;
    if (eglQuerySurface(display, surface, attribute, &value))
      return value;
    else
      return std::nullopt;
  }

  EGLBoolean SwapBuffers(EGLSurface surface) noexcept {
    return eglSwapBuffers(display, surface);
  }

private:
  void InitDisplay(EGLNativeDisplayType native_display);
  void CreateContext();
};

} // namespace EGL
