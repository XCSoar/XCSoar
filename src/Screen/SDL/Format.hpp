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

#ifndef XCSOAR_SCREEN_SDL_FORMAT_HPP
#define XCSOAR_SCREEN_SDL_FORMAT_HPP

#include "Compiler.h"

#include <SDL.h>

#include <assert.h>

#ifdef GREYSCALE
/**
 * Set by TopCanvas::Create().
 */
extern SDL_PixelFormat *greyscale_format;
#endif

gcc_const
static inline const SDL_PixelFormat &
GetDisplayFormat()
{
#ifdef GREYSCALE
  /* with the "GREYSCALE" option, we render to software 8-bit
     surfaces, and TopCanvas::Flip() will convert to the real video
     surface */
  return *greyscale_format;
#else
  SDL_Surface *surface = SDL_GetVideoSurface();
  assert(surface != nullptr);
  assert(surface->format != nullptr);

  return *surface->format;
#endif
}

gcc_pure
static inline bool
CompareFormats(const SDL_PixelFormat &a, const SDL_PixelFormat &b)
{
  return a.BitsPerPixel == b.BitsPerPixel &&
    a.Rmask == b.Rmask && a.Gmask == b.Gmask &&
    a.Bmask == b.Bmask && a.Amask == b.Amask;
}

gcc_pure
static inline bool
IsDisplayFormat(const SDL_PixelFormat &format)
{
  return CompareFormats(format, GetDisplayFormat());
}

/**
 * Returns the unmodified surface if it is compatible with the video
 * surface.  If not, the surface is converted, and the parameter is
 * freed.
 */
static inline SDL_Surface *
ConvertToDisplayFormat(SDL_Surface *surface)
{
  if (IsDisplayFormat(*surface->format))
    /* already using the display format */
    return surface;

  /* need to convert */
#ifdef GREYSCALE
  SDL_Surface *converted = SDL_ConvertSurface(surface, greyscale_format,
                                              SDL_SWSURFACE);
#else
  SDL_Surface *converted = SDL_DisplayFormat(surface);
#endif

  SDL_FreeSurface(surface);
  return converted;
}

/**
 * Like ConvertToDisplayFormat(), but does not destroy the old
 * surface.
 */
static inline SDL_Surface *
ConvertToDisplayFormatPreserve(SDL_Surface *surface)
{
  if (IsDisplayFormat(*surface->format))
    /* already using the display format */
    return surface;

  /* need to convert */
#ifdef GREYSCALE
  return SDL_ConvertSurface(surface, greyscale_format, SDL_SWSURFACE);
#else
  return SDL_DisplayFormat(surface);
#endif
}

#endif
