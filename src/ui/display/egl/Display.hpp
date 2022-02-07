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
