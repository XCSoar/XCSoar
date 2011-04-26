/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/Init.hpp"
#include "Screen/Debug.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Cache.hpp"
#include "Screen/OpenGL/Debug.hpp"
#endif

#include <SDL.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>

ScreenGlobalInit::ScreenGlobalInit()
{
  if (::SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
    fprintf(stderr, "SDL_Init() has failed\n");
    exit(EXIT_FAILURE);
  }

  ::SDL_EnableKeyRepeat(250, 50);

#if defined(ENABLE_OPENGL)
#ifndef NDEBUG
  OpenGL::thread = pthread_self();
#endif

  ::SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  ::SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
#endif

  if (::TTF_Init() != 0) {
    fprintf(stderr, "TTF_Init() has failed\n");
    exit(EXIT_FAILURE);
  }

  ScreenInitialized();
}

ScreenGlobalInit::~ScreenGlobalInit()
{
#ifdef ENABLE_OPENGL
  TextCache::flush();
#endif

  ::TTF_Quit();
  ::SDL_Quit();

  ScreenDeinitialized();
}
