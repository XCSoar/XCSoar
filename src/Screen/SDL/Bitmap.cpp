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

#include "Screen/Bitmap.hpp"
#include "Screen/Debug.hpp"
#include "ResourceLoader.hpp"
#include "OS/ConvertPathName.hpp"
#include "UncompressedImage.hpp"
#include "Screen/SDL/Format.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Debug.hpp"
#endif

#include <SDL_endian.h>

#ifdef WIN32
  #include <windows.h>
#endif

#include <assert.h>

bool
Bitmap::Load(SDL_Surface *_surface, Type type)
{
  assert(IsScreenInitialized());
  assert(_surface != NULL);

#ifdef ENABLE_OPENGL
  assert(texture == NULL);
  assert(pthread_equal(pthread_self(), OpenGL::thread));

  texture = new GLTexture(_surface);
  size.cx = _surface->w;
  size.cy = _surface->h;
  SDL_FreeSurface(_surface);

  return true;
#else
  assert(surface == NULL);

  switch (type) {
  case Type::STANDARD:
    surface = ConvertToDisplayFormat(_surface);
    break;

  case Type::MONO:
    // XXX convert?
    surface = _surface;

    assert(surface->format->palette != NULL &&
           surface->format->palette->ncolors == 256);
    assert(surface->format->palette->colors[0].r == 0);
    assert(surface->format->palette->colors[0].g == 0);
    assert(surface->format->palette->colors[0].b == 0);
    assert(surface->format->palette->colors[255].r == 255);
    assert(surface->format->palette->colors[255].g == 255);
    assert(surface->format->palette->colors[255].b == 255);
    break;
  }

  return true;
#endif
}

#ifndef ENABLE_OPENGL

bool
Bitmap::Load(const UncompressedImage &uncompressed, Type type)
{
  Reset();

  SDL_Surface *surface = ImportSurface(uncompressed);
  if (surface == nullptr)
    return false;

  return Load(surface, type);
}

#endif

bool
Bitmap::LoadStretch(unsigned id, unsigned zoom)
{
  assert(zoom > 0);

  // XXX
  return Load(id);
}

#ifndef ENABLE_OPENGL

void
Bitmap::Reset()
{
  assert(!IsDefined() || IsScreenInitialized());

  if (surface != NULL) {
    SDL_FreeSurface(surface);
    surface = NULL;
  }
}

const PixelSize
Bitmap::GetSize() const
{
  assert(IsDefined());

  return { surface->w, surface->h };
}

#endif /* !OpenGL */
