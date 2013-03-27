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

#include "UncompressedImage.hpp"
#include "Screen/Custom/UncompressedImage.hpp"
#include "Compiler.h"

#include <SDL.h>

SDL_Surface *
ImportSurface(const UncompressedImage &image)
{
  int depth;
  Uint32 rmask, gmask, bmask, amask;

  switch (image.GetFormat()) {
  case UncompressedImage::Format::GRAY:
    depth = 8;
    rmask = 0xff;
    gmask = 0xff;
    bmask = 0xff;
    amask = 0;
    break;

  case UncompressedImage::Format::RGB:
    depth = 24;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
#endif
    amask = 0;
    break;

  case UncompressedImage::Format::INVALID:
    return nullptr;

#ifdef __OPTIMIZE__
  default:
    gcc_unreachable();
#endif
  }

  SDL_Surface *src =
    SDL_CreateRGBSurfaceFrom(const_cast<void *>(image.GetData()),
                             image.GetWidth(), image.GetHeight(),
                             depth, image.GetPitch(),
                             rmask, gmask, bmask, amask);

  /* copy the SDL_Surface because the UncompressedImage::GetData()
     allocation will be freed soon, but SDL_Surface::pixels must
     survive that */
  SDL_Surface *copy = SDL_ConvertSurface(src, src->format, SDL_HWSURFACE);
  SDL_FreeSurface(src);
  return copy;
}
