// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "ui/canvas/opengl/Init.hpp"

namespace OpenGL {

Display::Display()
{
  Initialise();

  /* not calling SetupContext() here when using libSDL, because libSDL
     creates the OpenGL context using SDL_GL_CreateContext(), which
     requires already having a SDL_Window */
#ifndef ENABLE_SDL
  SetupContext();
#endif
}

Display::~Display() noexcept
{
  Deinitialise();
}

} // namespace OpenGL
