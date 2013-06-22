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

#ifdef DITHER
#include "Screen/Custom/Dither.hpp"
#endif

#include <SDL_video.h>

#include <assert.h>
#include <stdio.h>

#ifdef GREYSCALE
#include "Format.hpp"
SDL_PixelFormat *greyscale_format;
#endif

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
TopCanvas::Create(PixelSize new_size,
                  bool full_screen, bool resizable)
{
  const Uint32 flags = MakeSDLFlags(full_screen, resizable);

  SDL_Surface *s = ::SDL_SetVideoMode(new_size.cx, new_size.cy, 0, flags);
  if (s == NULL) {
    fprintf(stderr, "SDL_SetVideoMode(%u, %u, 0, %#x) has failed: %s\n",
            new_size.cx, new_size.cy, (unsigned)flags,
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
  OpenGL::SetupViewport(new_size.cx, new_size.cy);
  Canvas::Create(new_size);
#elif defined(GREYSCALE)
  real = s;

  s = ::SDL_CreateRGBSurface(SDL_SWSURFACE, new_size.cx, new_size.cy,
                             8, 0xff, 0xff, 0xff, 0);
  greyscale_format = s->format;
  Canvas::Create(s);
#else
  Canvas::Create(s);
#endif
}

void
TopCanvas::OnResize(PixelSize new_size)
{
  if (new_size == size)
    return;

#ifdef ENABLE_OPENGL
  const SDL_Surface *old = ::SDL_GetVideoSurface();
#else
  const SDL_Surface *old = surface;
#endif
  if (old == nullptr)
    return;

  const Uint32 flags = old->flags;

  SDL_Surface *s = ::SDL_SetVideoMode(new_size.cx, new_size.cy, 0, flags);
  if (s == NULL)
    return;

#ifdef ENABLE_OPENGL
  OpenGL::SetupViewport(new_size.cx, new_size.cy);
  Canvas::Create(new_size);
#else
#ifdef GREYSCALE
  real = s;

  s = ::SDL_CreateRGBSurface(SDL_SWSURFACE, new_size.cx, new_size.cy,
                             8, 0xff, 0xff, 0xff, 0);
  greyscale_format = s->format;
  Canvas::Destroy();
  surface = s;
#else
  surface = s;
#endif

  size = new_size;
#endif
}

void
TopCanvas::Fullscreen()
{
#if 0 /* disabled for now, for easier development */
  ::SDL_WM_ToggleFullScreen(surface);
#endif
}

#ifdef GREYSCALE

#ifdef DITHER

#else

static uint32_t
GreyscaleToRGB8(Luminosity8 luminosity)
{
  const unsigned value = luminosity.GetLuminosity();

  return value | (value << 8) | (value << 16) | (value << 24);
}

static void
CopyGreyscaleToRGB8(uint32_t *gcc_restrict dest,
                     const Luminosity8 *gcc_restrict src,
                     unsigned width)
{
  for (unsigned i = 0; i < width; ++i)
    *dest++ = GreyscaleToRGB8(*src++);
}

static RGB565Color
GreyscaleToRGB565(Luminosity8 luminosity)
{
  const unsigned value = luminosity.GetLuminosity();

  return RGB565Color(value, value, value);
}

static void
CopyGreyscaleToRGB565(RGB565Color *gcc_restrict dest,
                      const Luminosity8 *gcc_restrict src,
                      unsigned width)
{
  for (unsigned i = 0; i < width; ++i)
    *dest++ = GreyscaleToRGB565(*src++);
}

#endif

static void
CopyFromGreyscale(
#ifdef DITHER
                  Dither &dither,
#endif
                  SDL_Surface *dest, SDL_Surface *src)
{
  assert(src->format->BitsPerPixel == 8);
  assert(src->format->BytesPerPixel == 1);
  assert(dest->format->BytesPerPixel == 4 || dest->format->BytesPerPixel == 2);

  if (SDL_LockSurface(dest) != 0)
    return;

  if (SDL_LockSurface(src) != 0) {
    SDL_UnlockSurface(dest);
    return;
  }

  const uint8_t *src_pixels = (const uint8_t *)src->pixels;

  const unsigned width = dest->w, height = dest->h;

#ifdef DITHER

  dither.dither_luminosity8_to_uint16(src_pixels, src->pitch,
                                      (uint16_t *)dest->pixels,
                                      dest->pitch / dest->format->BytesPerPixel,
                                      width, height);
  if (dest->format->BytesPerPixel == 4) {
    const unsigned n_pixels = (dest->pitch / dest->format->BytesPerPixel)
      * height;
    int32_t *d = (int32_t *)dest->pixels + n_pixels;
    const int16_t *end = (int16_t *)dest->pixels;
    const int16_t *s = end + n_pixels;

    while (s != end)
      *--d = *--s;
  }

#else

  uint8_t *dest_pixels = (uint8_t *)dest->pixels;
  const unsigned src_pitch = src->pitch;
  const unsigned dest_pitch = dest->pitch;

  if (dest->format->BytesPerPixel == 2) {
    for (unsigned row = height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      CopyGreyscaleToRGB565((RGB565Color *)dest_pixels,
                            (const Luminosity8 *)src_pixels, width);
  } else {
    for (unsigned row = height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      CopyGreyscaleToRGB8((uint32_t *)dest_pixels,
                           (const Luminosity8 *)src_pixels, width);
  }

#endif

  ::SDL_UnlockSurface(src);
  ::SDL_UnlockSurface(dest);
}

#endif

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

#ifdef GREYSCALE
  CopyFromGreyscale(
#ifdef DITHER
                    dither,
#endif
                    real, surface);
  ::SDL_Flip(real);
#else
  ::SDL_Flip(surface);
#endif

#endif
}
