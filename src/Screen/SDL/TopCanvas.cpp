/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Screen/Point.hpp"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Init.hpp"
#include "Screen/OpenGL/Features.hpp"
#include "Math/Point2D.hpp"
#else
#include "Screen/Canvas.hpp"
#endif

#ifdef DITHER
#include "Screen/Memory/Dither.hpp"
#endif

#include <SDL_platform.h>
#include <SDL_version.h>
#include <SDL_video.h>
#if SDL_MAJOR_VERSION >= 2
#include <SDL_hints.h>
#ifdef USE_MEMORY_CANVAS
#include <SDL_render.h>
#endif
#if defined(__MACOSX__) && __MACOSX__
#include <SDL_syswm.h>
#import <AppKit/AppKit.h>
#include <alloca.h>
#endif
#endif

#include <assert.h>
#include <stdio.h>

#ifndef ENABLE_OPENGL

PixelRect
TopCanvas::GetRect() const
{
  assert(IsDefined());

#if SDL_MAJOR_VERSION >= 2
  int width, height;
  ::SDL_GetWindowSize(window, &width, &height);
  return { 0, 0, width, height };
#else
  return { 0, 0, surface->w, surface->h };
#endif
}

#endif

gcc_const
static Uint32
MakeSDLFlags(bool full_screen, bool resizable)
{
  Uint32 flags = 0;
#if SDL_MAJOR_VERSION < 2
  flags = SDL_ANYFORMAT;
#endif

#ifdef ENABLE_OPENGL
#if SDL_MAJOR_VERSION >= 2
  flags |= SDL_WINDOW_OPENGL;
#else
  flags |= SDL_OPENGL;
#endif
#else /* !ENABLE_OPENGL */
#if SDL_MAJOR_VERSION >= 2
  flags |= SDL_SWSURFACE;
#else
  /* we need async screen updates as long as we don't have a global
     frame rate */
  flags |= SDL_ASYNCBLIT;

  const SDL_VideoInfo *info = SDL_GetVideoInfo();
  assert(info != nullptr);

  if (info->hw_available)
    flags |= SDL_HWSURFACE;
  else
    flags |= SDL_SWSURFACE;
#endif /* !SDL_MAJOR_VERSION */
#endif /* !ENABLE_OPENGL */

#if !defined(__MACOSX__) || !(__MACOSX__)
  if (full_screen)
#if SDL_MAJOR_VERSION >= 2
    flags |= SDL_WINDOW_FULLSCREEN;
#else
    flags |= SDL_FULLSCREEN;
#endif
#endif

  if (resizable)
#if SDL_MAJOR_VERSION >= 2
    flags |= SDL_WINDOW_RESIZABLE;
#else
    flags |= SDL_RESIZABLE;
#endif

#if defined(__IPHONEOS__) && __IPHONEOS__
  /* Hide status bar on iOS devices */
  flags |= SDL_WINDOW_BORDERLESS;
#endif

#ifdef HAVE_HIGHDPI_SUPPORT
  flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif

  return flags;
}

#if SDL_MAJOR_VERSION >= 2
void
TopCanvas::Create(const char *text, PixelSize new_size,
                  bool full_screen, bool resizable)
#else
void
TopCanvas::Create(PixelSize new_size,
                  bool full_screen, bool resizable)
#endif
{
  const Uint32 flags = MakeSDLFlags(full_screen, resizable);

#if SDL_MAJOR_VERSION >= 2
  window = ::SDL_CreateWindow(text, SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, new_size.cx,
                              new_size.cy, flags);
  if (window == nullptr) {
    fprintf(stderr,
            "SDL_CreateWindow(%s, %u, %u, %u, %u, %#x) has failed: %s\n",
            text, (unsigned) SDL_WINDOWPOS_UNDEFINED,
            (unsigned) SDL_WINDOWPOS_UNDEFINED, (unsigned) new_size.cx,
            (unsigned) new_size.cy, (unsigned)flags,
            ::SDL_GetError());
    return;
  }

#if defined(__MACOSX__) && __MACOSX__
  SDL_SysWMinfo *wm_info =
      reinterpret_cast<SDL_SysWMinfo *>(alloca(sizeof(SDL_SysWMinfo)));
  SDL_VERSION(&wm_info->version);
  if ((SDL_GetWindowWMInfo(window, wm_info)) &&
      (wm_info->subsystem == SDL_SYSWM_COCOA)) {
    if (resizable) {
      [wm_info->info.cocoa.window
          setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];
    }
    if (full_screen) {
      [wm_info->info.cocoa.window toggleFullScreen: nil];
    }
  }
#endif

#ifdef USE_MEMORY_CANVAS
  renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == nullptr) {
    fprintf(stderr,
            "SDL_CreateRenderer(%p, %d, %d) has failed: %s\n",
            window, -1, 0, ::SDL_GetError());
        return;
  }
  int width, height;
  SDL_GetWindowSize(window, &width, &height);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                              SDL_TEXTUREACCESS_STREAMING,
                              width, height);
  if (texture == nullptr) {
      fprintf(stderr,
              "SDL_CreateTexture(%p, %d, %d, %d, %d) has failed: %s\n",
              renderer, (int) SDL_PIXELFORMAT_UNKNOWN,
              (int) SDL_TEXTUREACCESS_STREAMING, width, height,
              ::SDL_GetError());
          return;
    }
#endif

#else
  SDL_Surface *s = ::SDL_SetVideoMode(new_size.cx, new_size.cy, 0, flags);
  if (s == nullptr) {
    fprintf(stderr, "SDL_SetVideoMode(%u, %u, 0, %#x) has failed: %s\n",
            new_size.cx, new_size.cy, (unsigned)flags,
            ::SDL_GetError());
    return;
  }
#endif

#ifdef ENABLE_OPENGL
#if SDL_MAJOR_VERSION >= 2
  if (::SDL_GL_CreateContext(window) == nullptr) {
    fprintf(stderr, "SDL_GL_CreateContext(%p) has failed: %s\n",
            window, ::SDL_GetError());
    return;
  }
#else
  if (full_screen)
    /* after a X11 mode switch to full-screen mode, the first
       SDL_GL_SwapBuffers() call gets ignored; could be a SDL 1.2 bug, and
       the following dummy call works around it: */
    ::SDL_GL_SwapBuffers();
#endif

  OpenGL::SetupContext();
#ifdef HAVE_HIGHDPI_SUPPORT
  int gl_width, gl_height;
  SDL_GL_GetDrawableSize(window, &gl_width, &gl_height);
  OpenGL::SetupViewport(UnsignedPoint2D(gl_width, gl_height));
  Canvas::Create(PixelSize(gl_width, gl_height));
#else
  OpenGL::SetupViewport(UnsignedPoint2D(new_size.cx, new_size.cy));
  Canvas::Create(new_size);
#endif
#elif (SDL_MAJOR_VERSION < 2)
  surface = s;
#endif

#ifdef GREYSCALE
  buffer.Allocate(new_size.cx, new_size.cy);
#endif
}

void
TopCanvas::Destroy()
{
#if !defined(ENABLE_OPENGL) && defined(GREYSCALE)
  buffer.Free();
#endif

#if defined(USE_MEMORY_CANVAS) && (SDL_MAJOR_VERSION >= 2)
  SDL_DestroyTexture(texture);
#endif
}

void
TopCanvas::OnResize(PixelSize new_size)
{
#ifdef ENABLE_OPENGL
  if (new_size == GetSize())
    return;

#if SDL_MAJOR_VERSION < 2
  const SDL_Surface *old = ::SDL_GetVideoSurface();
#endif
#else
#if SDL_MAJOR_VERSION >= 2
  int texture_width, texture_height;
  Uint32 texture_format;
  if (SDL_QueryTexture(texture, &texture_format, NULL, &texture_width, &texture_height) != 0)
    return;
  if (new_size.cx == texture_width && new_size.cy == texture_height)
    return;
#else
  if (new_size.cx == surface->w && new_size.cy == surface->h)
    return;
#endif

#if SDL_MAJOR_VERSION >= 2
  SDL_Texture *t = SDL_CreateTexture(renderer, texture_format,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     new_size.cx, new_size.cy);
  if (t == nullptr)
    return;

#else
  const SDL_Surface *old = surface;
#endif
#endif

#if SDL_MAJOR_VERSION < 2
  if (old == nullptr)
    return;

  const Uint32 flags = old->flags;

  SDL_Surface *s = ::SDL_SetVideoMode(new_size.cx, new_size.cy, 0, flags);
  if (s == nullptr)
    return;
#endif

#ifdef ENABLE_OPENGL
  OpenGL::SetupViewport(UnsignedPoint2D(new_size.cx, new_size.cy));
  Canvas::Create(new_size);
#else
#if (SDL_MAJOR_VERSION >= 2)
  if (texture != nullptr)
      SDL_DestroyTexture(texture);
  texture = t;
#else
  surface = s;
#endif
#endif

#ifdef GREYSCALE
  buffer.Free();
  buffer.Allocate(new_size.cx, new_size.cy);
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

#if CLANG_OR_GCC_VERSION(4,8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#endif

static void
CopyFromGreyscale(
#ifdef DITHER
                  Dither &dither,
#endif
#if SDL_MAJOR_VERSION >= 2
                  SDL_Texture *dest,
#else
                  SDL_Surface *dest,
#endif
                  ConstImageBuffer<GreyscalePixelTraits> src)
{
#if SDL_MAJOR_VERSION < 2
  int bytes_per_pixel = dest->format->BytesPerPixel;
#endif

#if SDL_MAJOR_VERSION >= 2
  uint8_t *dest_pixels;
  int pitch_as_int, dest_with, dest_height;
  SDL_QueryTexture(dest, nullptr, nullptr, &dest_with, &dest_height);
  if (SDL_LockTexture(dest, nullptr,
                      reinterpret_cast<void**>(&dest_pixels),
                      &pitch_as_int) != 0)
    return;

  int bytes_per_pixel = pitch_as_int / dest_with;
#else
  if (SDL_LockSurface(dest) != 0)
    return;
#endif

  assert(bytes_per_pixel == 4 || bytes_per_pixel == 2);

  const uint8_t *src_pixels = reinterpret_cast<const uint8_t *>(src.data);

  const unsigned width = src.width, height = src.height;

#if SDL_MAJOR_VERSION >= 2
  const unsigned dest_pitch = (unsigned) pitch_as_int;
#else
  uint8_t *dest_pixels = (uint8_t *)dest->pixels;
  const unsigned dest_pitch = dest->pitch;
#endif

#ifdef DITHER

  dither.DitherGreyscale(src_pixels, src.pitch,
                         dest_pixels,
                         dest_pitch / bytes_per_pixel,
                         width, height);
  if (bytes_per_pixel == 4) {
    const unsigned n_pixels = (dest_pitch / bytes_per_pixel)
      * height;
    int32_t *d = (int32_t *)dest_pixels + n_pixels;
    const int8_t *end = (int8_t *)dest_pixels;
    const int8_t *s = end + n_pixels;

    while (s != end)
      *--d = *--s;
  }

#else

  const unsigned src_pitch = src.pitch;

  if (bytes_per_pixel == 2) {
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

#if SDL_MAJOR_VERSION >= 2
  ::SDL_UnlockTexture(dest);
#else
  ::SDL_UnlockSurface(dest);
#endif
}

#if CLANG_OR_GCC_VERSION(4,8)
#pragma GCC diagnostic pop
#endif

#endif

#ifndef ENABLE_OPENGL

Canvas
TopCanvas::Lock()
{
#ifndef GREYSCALE
#if SDL_MAJOR_VERSION >= 2
  WritableImageBuffer<ActivePixelTraits> buffer;
  void* pixels;
  int pitch, width, height;
  SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
  if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0)
    return Canvas();
  buffer.data = (ActivePixelTraits::pointer_type)pixels;
  buffer.pitch = (unsigned) pitch;
  buffer.width = (unsigned) width;
  buffer.height = (unsigned) height;
#else
  if (SDL_LockSurface(surface) != 0)
    return Canvas();

  WritableImageBuffer<ActivePixelTraits> buffer;
  buffer.data = (ActivePixelTraits::pointer_type)surface->pixels;
  buffer.pitch = surface->pitch;
  buffer.width = surface->w;
  buffer.height = surface->h;
#endif
#endif

  return Canvas(buffer);
}

void
TopCanvas::Unlock()
{
#ifndef GREYSCALE
#if SDL_MAJOR_VERSION >= 2
  SDL_UnlockTexture(texture);
#else
  SDL_UnlockSurface(surface);
#endif
#endif
}

#endif

void
TopCanvas::Flip()
{
#ifdef ENABLE_OPENGL
#if SDL_MAJOR_VERSION >= 2
  ::SDL_GL_SwapWindow(window);
#else
  ::SDL_GL_SwapBuffers();
#endif
#else

#ifdef GREYSCALE
  CopyFromGreyscale(
#ifdef DITHER
                    dither,
#endif
#if SDL_MAJOR_VERSION >= 2
                    texture, buffer);
#else
                    surface, buffer);
#endif
#endif

#if SDL_MAJOR_VERSION >= 2
  ::SDL_RenderClear(renderer);
  ::SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  ::SDL_RenderPresent(renderer);
#else
  ::SDL_Flip(surface);
#endif

#endif
}
