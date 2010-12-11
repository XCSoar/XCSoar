/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Cache.hpp"
#endif

#include <SDL.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>

#if defined(ANDROID)
/* temporary hack */
extern "C" {
  int SDL_StartEventLoop(Uint32 flags);
}
struct SDL_VideoDevice {
  char dummy[0xbc];
  void (*InitOSKeymap)(void *dummy);
};
extern SDL_VideoDevice *current_video;
static void Dummy(void *dummy) {}
#endif

ScreenGlobalInit::ScreenGlobalInit()
{
#if defined(ANDROID)
  /* temporary hack */
  SDL_VideoDevice dummy;
  void *p = &dummy;
  char *q = (char *)p;
  for (unsigned i = 0; i < sizeof(dummy); ++i)
    q[i] = (unsigned char)i;
  dummy.InitOSKeymap = Dummy;
  current_video = &dummy;
  if (::SDL_Init(SDL_INIT_TIMER) != 0 ||
      ::SDL_StartEventLoop(0) != 0) {
    fprintf(stderr, "SDL_Init() has failed\n");
    exit(EXIT_FAILURE);
  }
  current_video = NULL;
#else
  if (::SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
    fprintf(stderr, "SDL_Init() has failed\n");
    exit(EXIT_FAILURE);
  }
#endif

#if defined(ENABLE_OPENGL) && !defined(ANDROID)
  ::SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  ::SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
#endif

#ifndef ANDROID
  if (::TTF_Init() != 0) {
    fprintf(stderr, "TTF_Init() has failed\n");
    exit(EXIT_FAILURE);
  }
#endif
}

ScreenGlobalInit::~ScreenGlobalInit()
{
#ifdef ENABLE_OPENGL
  TextCache::flush();
#endif

#ifndef ANDROID
  ::TTF_Quit();
#endif
  ::SDL_Quit();
}
