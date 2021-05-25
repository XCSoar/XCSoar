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

#include "../Init.hpp"
#include "Screen/Debug.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#include "util/RuntimeError.hxx"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Init.hpp"
#endif

#include <SDL.h>
#include <SDL_hints.h>

using namespace UI;

ScreenGlobalInit::ScreenGlobalInit()
{
  Uint32 flags = SDL_INIT_VIDEO;
  if (!IsKobo())
    flags |= SDL_INIT_AUDIO;

  if (::SDL_Init(flags) != 0)
    throw FormatRuntimeError("SDL_Init() has failed: %s", ::SDL_GetError());

#ifdef HAVE_GLES
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

  OpenGL::Initialise();
#endif

#ifdef USE_FREETYPE
  Font::Initialise();
#endif

  event_queue = new EventQueue();

  ScreenInitialized();
}

ScreenGlobalInit::~ScreenGlobalInit()
{
  delete event_queue;
  event_queue = nullptr;

#ifdef ENABLE_OPENGL
  OpenGL::Deinitialise();
#endif

#ifdef USE_FREETYPE
  Font::Deinitialise();
#endif

  ::SDL_Quit();

  ScreenDeinitialized();
}
