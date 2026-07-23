// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "Asset.hpp"
#include "Math/Point2D.hpp"

#include <SDL.h>
#include <SDL_hints.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

namespace SDL {

Display::Display()
{
#ifdef _WIN32
  SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS,
              "permonitorv2");
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
  SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#endif

  Uint32 flags = SDL_INIT_VIDEO;
  if (!IsKobo())
    flags |= SDL_INIT_AUDIO;

  if (::SDL_Init(flags) != 0)
    throw FmtRuntimeError("SDL_Init() has failed: {}", ::SDL_GetError());

#ifdef ENABLE_OPENGL
#if defined(USE_ANGLE)
  // On Windows, tell SDL to use EGL (required for ANGLE)
  SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#endif
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

  // Keep screen on (works on iOS, and maybe for other platforms)
  SDL_SetHint(SDL_HINT_IDLE_TIMER_DISABLED, "1");

  if (HasTouchScreen())
    SDL_ShowCursor (SDL_FALSE);

#if defined(ENABLE_OPENGL)
  ::SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#if defined(__APPLE__) && TARGET_OS_IPHONE
  ::SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
#else
  ::SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
#endif
#endif
}

Display::~Display() noexcept
{
  ::SDL_Quit();
}

UnsignedPoint2D
Display::GetDPI() noexcept
{
  float ddpi, hdpi, vdpi;

  if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) == 0)
    return {static_cast<unsigned>(hdpi), static_cast<unsigned>(vdpi)};

  return {96, 96};
}

} // namespace SDL
