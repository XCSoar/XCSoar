/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Screen/Custom/TopCanvas.hpp"
#include "Screen/Features.hpp"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Init.hpp"
#include "Screen/OpenGL/Features.hpp"
#ifdef HAVE_EGL
#include "Screen/OpenGL/EGL.hpp"
#include "Screen/OpenGL/Globals.hpp"
#endif
#endif

#include <assert.h>
#include <stdio.h>

gcc_const
static Uint32
MakeSDLFlags(bool full_screen, bool resizable)
{
  Uint32 flags = SDL_ANYFORMAT;

#ifdef ENABLE_OPENGL
  flags |= SDL_OPENGL;
#else /* !ENABLE_OPENGL */
  /* we need async screen updates as long as we don't have a global
     frame rate */
  flags |= SDL_ASYNCBLIT;

  const SDL_VideoInfo *info = SDL_GetVideoInfo();
  assert(info != NULL);

  if (info->hw_available)
    flags |= SDL_HWSURFACE;
  else
    flags |= SDL_SWSURFACE;
#endif /* !ENABLE_OPENGL */

  if (full_screen)
    flags |= SDL_FULLSCREEN;

  if (resizable)
    flags |= SDL_RESIZABLE;

  return flags;
}

void
TopCanvas::Create(UPixelScalar width, UPixelScalar height,
                  bool full_screen, bool resizable)
{
  flags = MakeSDLFlags(full_screen, resizable);

  SDL_Surface *s = ::SDL_SetVideoMode(width, height, 0, flags);
  if (s == NULL) {
    fprintf(stderr, "SDL_SetVideoMode(%u, %u, 0, %#x) has failed: %s\n",
            width, height, (unsigned)flags,
            ::SDL_GetError());
    return;
  }

#ifdef ENABLE_OPENGL
  if (full_screen)
    /* after a X11 mode switch to full-screen mode, the first
       SDL_GL_SwapBuffers() call gets ignored; could be a SDL bug, and
       the following dummy call works around it: */
    ::SDL_GL_SwapBuffers();

  OpenGL::SetupContext();
  OpenGL::SetupViewport(width, height);
  Canvas::Create(width, height);
#else
  Canvas::Create(s);
#endif
}

void
TopCanvas::OnResize(UPixelScalar width, UPixelScalar height)
{
  if (width == GetWidth() && height == GetHeight())
    return;

  SDL_Surface *s = ::SDL_SetVideoMode(width, height, 0, flags);
  if (s == NULL)
    return;

#ifdef ENABLE_OPENGL
  OpenGL::SetupViewport(width, height);
  Canvas::Create(width, height);
#endif
}

void
TopCanvas::Fullscreen()
{
#if 0 /* disabled for now, for easier development */
  ::SDL_WM_ToggleFullScreen(surface);
#endif
}

void
TopCanvas::Flip()
{
#ifdef ENABLE_OPENGL
#ifdef HAVE_EGL
  if (OpenGL::egl) {
    /* if native EGL support was detected, we can circumvent the JNI
       call */
    EGLSwapBuffers();
    return;
  }
#endif

  ::SDL_GL_SwapBuffers();
#else
  ::SDL_Flip(surface);
#endif
}
