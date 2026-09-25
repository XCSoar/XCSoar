// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "Asset.hpp"
#include "Math/Point2D.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_hints.h>

namespace SDL {

Display::Display()
{
  Uint32 flags = SDL_INIT_VIDEO;
  if (!IsKobo())
    flags |= SDL_INIT_AUDIO;

  if (!::SDL_Init(flags))
    throw FmtRuntimeError("SDL_Init() has failed: {}", ::SDL_GetError());

#ifdef ENABLE_OPENGL
#ifdef USE_ANGLE
  // On Windows, tell SDL to use EGL (required for ANGLE)
  SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#ifdef __APPLE__
  // SDL's EGL loader does not search the app bundle for bare dylib names.
  SDL_SetHint(SDL_HINT_OPENGL_LIBRARY, "@rpath/libGLESv2.dylib");
  SDL_SetHint(SDL_HINT_EGL_LIBRARY, "@rpath/libEGL.dylib");
#endif
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

  // Keep screen on (works on iOS, and maybe for other platforms)
  SDL_DisableScreenSaver();

  if (HasTouchScreen())
    SDL_HideCursor();

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
Display::GetDPI() noexcept
{
  const float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  const unsigned dpi = scale > 0 ? unsigned(scale * 96) : 96;
  return {dpi, dpi};
}

} // namespace SDL
