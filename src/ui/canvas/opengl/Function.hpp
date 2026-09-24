// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_EGL
#include "ui/egl/System.hpp"
#elif defined(ENABLE_SDL)
#include <SDL_video.h>
#else
#include <dlfcn.h>
#endif

namespace OpenGL {

typedef void (*Function)();

static inline Function
GetProcAddress(const char *name)
{
#ifdef USE_EGL
  return eglGetProcAddress(name);
#elif defined(ENABLE_SDL)
  return (Function)SDL_GL_GetProcAddress(name);
#else
  return (Function)dlsym(RTLD_DEFAULT, name);
#endif
}

} // namespace OpenGL
