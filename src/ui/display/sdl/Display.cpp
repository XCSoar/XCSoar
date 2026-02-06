// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "Asset.hpp"
#include "Math/Point2D.hpp"

#include <SDL.h>
#include <SDL_hints.h>

namespace SDL {

Display::Display()
{
  Uint32 flags = SDL_INIT_VIDEO;
  if (!IsKobo())
    flags |= SDL_INIT_AUDIO;

  if (::SDL_Init(flags) != 0)
    throw FmtRuntimeError("SDL_Init() has failed: {}", ::SDL_GetError());

#ifdef ENABLE_OPENGL
#ifdef HAVE_GLES2
  // Request OpenGL ES 2.0 profile (ANGLE or native GLES)
#ifdef USE_ANGLE
  // On Windows, tell SDL to use EGL (required for ANGLE)
  SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
  // Request desktop OpenGL compatibility profile
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
#endif

  // Keep screen on (works on iOS, and maybe for other platforms)
  SDL_SetHint(SDL_HINT_IDLE_TIMER_DISABLED, "1");

  if (HasTouchScreen())
    SDL_ShowCursor (SDL_FALSE);

#if defined(ENABLE_OPENGL)
  ::SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  ::SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
#endif
}

Display::~Display() noexcept
{
  ::SDL_Quit();
}

UnsignedPoint2D
Display::GetDPI() const noexcept
{
  float ddpi, hdpi, vdpi;

  if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) == 0)
    return {static_cast<unsigned>(hdpi), static_cast<unsigned>(vdpi)};

  return {96, 96};
}

} // namespace SDL
