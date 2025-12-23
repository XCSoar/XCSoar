// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "Asset.hpp"

#include <SDL.h>
#include <SDL_hints.h>

namespace SDL {

Display::Display(unsigned antialiasing_samples)
{
  Uint32 flags = SDL_INIT_VIDEO;
  if (!IsKobo())
    flags |= SDL_INIT_AUDIO;

  if (::SDL_Init(flags) != 0)
    throw FmtRuntimeError("SDL_Init() has failed: {}", ::SDL_GetError());

#ifdef ENABLE_OPENGL
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

  // Keep screen on (works on iOS, and maybe for other platforms)
  SDL_SetHint(SDL_HINT_IDLE_TIMER_DISABLED, "1");

  if (HasTouchScreen())
    SDL_ShowCursor (SDL_FALSE);

#if defined(ENABLE_OPENGL)
  ::SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  ::SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
  if (antialiasing_samples > 0) {
    ::SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    ::SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, antialiasing_samples);
  }
#endif
}

Display::~Display() noexcept
{
  ::SDL_Quit();
}

void
Display::DisableAntiAliasing() noexcept
{
#if defined(ENABLE_OPENGL)
  ::SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
  ::SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
#endif
}

} // namespace SDL
