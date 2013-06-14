/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/VirtualCanvas.hpp"

#include <assert.h>

#ifdef ENABLE_SDL

VirtualCanvas::VirtualCanvas(PixelSize new_size)
{
  Create(new_size);
}

VirtualCanvas::VirtualCanvas(const Canvas &canvas, PixelSize new_size)
{
  Create(new_size);
}

#endif /* !ENABLE_SDL */

void
VirtualCanvas::Create(PixelSize new_size)
{
  assert((PixelScalar)new_size.cx >= 0);
  assert((PixelScalar)new_size.cy >= 0);

  Destroy();

#ifdef ENABLE_SDL
  const SDL_Surface *video = ::SDL_GetVideoSurface();
  assert(video != NULL);
  const SDL_PixelFormat *format = video->format;

  SDL_Surface *surface;
  surface = ::SDL_CreateRGBSurface(SDL_SWSURFACE, new_size.cx, new_size.cy,
                                   format->BitsPerPixel,
                                   format->Rmask, format->Gmask,
                                   format->Bmask, format->Amask);
  if (surface != NULL)
    Canvas::Create(surface);
#else /* !ENABLE_SDL */
  Canvas::Create(CreateCompatibleDC(NULL), new_size);
#endif /* !ENABLE_SDL */
}

void
VirtualCanvas::Create(const Canvas &canvas, PixelSize new_size)
{
  assert(canvas.IsDefined());

#ifdef ENABLE_SDL
  Create(new_size);
#else /* !ENABLE_SDL */
  Destroy();
  Canvas::Create(CreateCompatibleDC(canvas), new_size);
#endif /* !ENABLE_SDL */
}

void
VirtualCanvas::Create(const Canvas &canvas)
{
  Create(canvas, canvas.GetSize());
}

#ifndef ENABLE_SDL
void VirtualCanvas::Destroy()
{
  Canvas::Destroy();

  if (dc != NULL)
    ::DeleteDC(dc);
}
#endif /* !ENABLE_SDL */
