// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "ui/canvas/opengl/Init.hpp"
#include "ui/canvas/opengl/Globals.hpp"

namespace OpenGL {

Display::Display(unsigned antialiasing_samples)
{
  Initialise();

  /* Store the configured antialiasing samples so that BufferWindow
     can paint accordingly */
  OpenGL::antialiasing_samples = antialiasing_samples;

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
