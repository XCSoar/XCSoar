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

#include "Display.hpp"
#include "ConfigChooser.hpp"
#include "ui/opengl/Features.hpp"
#include "util/RuntimeError.hxx"
#include "LogFile.hpp"

#include <cassert>

/**
 * Returns the EGL API to bind to using eglBindAPI().
 */
static constexpr EGLenum
GetBindAPI()
{
  return HaveGLES()
    ? EGL_OPENGL_ES_API
    : EGL_OPENGL_API;
}

namespace EGL {

Display::Display(EGLNativeDisplayType native_display)
{
  InitDisplay(native_display);
  CreateContext();
}

Display::~Display() noexcept
{
#ifndef ANDROID
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext(display, context);
  eglTerminate(display);
#endif
}

inline void
Display::InitDisplay(EGLNativeDisplayType native_display)
{
  assert(display == EGL_NO_DISPLAY);

  display = eglGetDisplay(native_display);
  if (display == EGL_NO_DISPLAY)
    throw std::runtime_error("eglGetDisplay(EGL_DEFAULT_DISPLAY) failed");

  if (!eglInitialize(display, nullptr, nullptr))
    throw std::runtime_error("eglInitialize() failed");

  if (const char *s = eglQueryString(display, EGL_VENDOR))
    LogFormat("EGL vendor: %s", s);

  if (const char *s = eglQueryString(display, EGL_VERSION))
    LogFormat("EGL version: %s", s);

  if (const char *s = eglQueryString(display, EGL_EXTENSIONS))
    LogFormat("EGL extensions: %s", s);

  if (!eglBindAPI(GetBindAPI()))
    throw std::runtime_error("eglBindAPI() failed");

  chosen_config = EGL::ChooseConfig(display);
}

inline void
Display::CreateContext()
{
  assert(display != EGL_NO_DISPLAY);
  assert(context == EGL_NO_CONTEXT);

#ifdef HAVE_GLES2
  static constexpr EGLint context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
#else
  const EGLint *context_attributes = nullptr;
#endif

  context = eglCreateContext(display, chosen_config,
                             EGL_NO_CONTEXT, context_attributes);
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
}

EGLSurface
Display::CreateWindowSurface(EGLNativeWindowType native_window)
{
  auto surface = eglCreateWindowSurface(display, chosen_config,
                                        native_window, nullptr);
  if (surface == EGL_NO_SURFACE)
    throw FormatRuntimeError("eglCreateWindowSurface() failed: %#x",
                             eglGetError());

  return surface;
}

void
Display::MakeCurrent(EGLSurface surface)
{
  if (!eglMakeCurrent(display, surface, surface, context))
    throw std::runtime_error("eglMakeCurrent() failed");
}

} // namespace EGL
