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

#include "Screen/SDL/TopCanvas.hpp"
#include "Screen/Features.hpp"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Init.hpp"
#endif

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#include <assert.h>

void
TopCanvas::set(unsigned width, unsigned height)
{
#ifndef ANDROID
  Uint32 flags = SDL_ANYFORMAT;
#endif

#ifdef ENABLE_OPENGL
#ifndef ANDROID
  flags |= SDL_OPENGL;
#endif
#else /* !ENABLE_OPENGL */
  /* double buffering temporarily disabled on Android because
     Android's SDL port doesn't allow locking it then (which we need
     for SDL_gfx) */
  if (!is_android())
    flags |= SDL_DOUBLEBUF;

  /* we need async screen updates as long as we don't have a global
     frame rate */
  flags |= SDL_ASYNCBLIT;

  const SDL_VideoInfo *info = SDL_GetVideoInfo();
  assert(info != NULL);

  /* hardware surface temporarily disabled on Android because
     Android's SDL port doesn't allow locking it then (which we need
     for SDL_gfx) */
  if (!is_android() && info->hw_available)
    flags |= SDL_HWSURFACE;
  else
    flags |= SDL_SWSURFACE;
#endif /* !ENABLE_OPENGL */

  if (is_embedded()) {
#if defined(ANDROID)
    width = native_view->get_width();
    height = native_view->get_height();
#else
    flags |= SDL_FULLSCREEN;

    /* select a full-screen video mode */
    SDL_Rect **modes = SDL_ListModes(NULL, flags);
    if (modes == NULL)
      return;

    width = modes[0]->w;
    height = modes[0]->h;
#endif
  }

#ifdef ENABLE_OPENGL
#ifndef ANDROID
  ::SDL_SetVideoMode(width, height, 0, flags);
#endif
  OpenGL::SetupContext();
  OpenGL::SetupViewport(width, height);
  Canvas::set(width, height);
#else
  Canvas::set(::SDL_SetVideoMode(width, height, 0, flags));
#endif

#ifdef ENABLE_OPENGL
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_DITHER);
  glDisable(GL_LIGHTING);

  glEnableClientState(GL_VERTEX_ARRAY);
#endif
}

void
TopCanvas::full_screen()
{
#if 0 /* disabled for now, for easier development */
  ::SDL_WM_ToggleFullScreen(surface);
#endif
}

void
TopCanvas::flip()
{
#ifdef ANDROID
  native_view->swap();
#elif defined(ENABLE_OPENGL)
  ::SDL_GL_SwapBuffers();
#else
  ::SDL_Flip(surface);
#endif
}
