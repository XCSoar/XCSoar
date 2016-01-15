/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Screen/Memory/Export.hpp"
#include "Screen/Canvas.hpp"
#endif

#ifdef DITHER
#include "Screen/Memory/Dither.hpp"
#endif

#include <SDL_platform.h>
#include <SDL_video.h>
#include <SDL_hints.h>
#ifdef USE_MEMORY_CANVAS
#include <SDL_render.h>
#endif
#if defined(__MACOSX__) && __MACOSX__
#include <SDL_syswm.h>
#import <AppKit/AppKit.h>
#include <alloca.h>
#endif

#include <assert.h>
#include <stdio.h>

#ifndef ENABLE_OPENGL

PixelRect
TopCanvas::GetRect() const
{
  assert(IsDefined());

  int width, height;
  ::SDL_GetWindowSize(window, &width, &height);
  return { 0, 0, width, height };
}

#endif

gcc_const
static Uint32
MakeSDLFlags(bool full_screen, bool resizable)
{
  Uint32 flags = 0;

#ifdef ENABLE_OPENGL
  flags |= SDL_WINDOW_OPENGL;
#else /* !ENABLE_OPENGL */
  flags |= SDL_SWSURFACE;
#endif /* !ENABLE_OPENGL */

#if !defined(__MACOSX__) || !(__MACOSX__)
  if (full_screen)
    flags |= SDL_WINDOW_FULLSCREEN;
#endif

  if (resizable)
    flags |= SDL_WINDOW_RESIZABLE;

#if defined(__IPHONEOS__) && __IPHONEOS__
  /* Hide status bar on iOS devices */
  flags |= SDL_WINDOW_BORDERLESS;
#endif

#ifdef HAVE_HIGHDPI_SUPPORT
  flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif

  return flags;
}

void
TopCanvas::Create(const char *text, PixelSize new_size,
                  bool full_screen, bool resizable)
{
  const Uint32 flags = MakeSDLFlags(full_screen, resizable);

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

#ifdef ENABLE_OPENGL
  if (::SDL_GL_CreateContext(window) == nullptr) {
    fprintf(stderr, "SDL_GL_CreateContext(%p) has failed: %s\n",
            window, ::SDL_GetError());
    return;
  }

  OpenGL::SetupContext();
  int gl_width, gl_height;
  SDL_GL_GetDrawableSize(window, &gl_width, &gl_height);
  OpenGL::SetupViewport(UnsignedPoint2D(gl_width, gl_height));
  Canvas::Create(PixelSize(gl_width, gl_height));
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

#ifdef USE_MEMORY_CANVAS
  SDL_DestroyTexture(texture);
#endif
}

#ifdef USE_MEMORY_CANVAS

void
TopCanvas::OnResize(PixelSize new_size)
{
  int texture_width, texture_height;
  Uint32 texture_format;
  if (SDL_QueryTexture(texture, &texture_format, NULL, &texture_width, &texture_height) != 0)
    return;
  if (new_size.cx == texture_width && new_size.cy == texture_height)
    return;

  SDL_Texture *t = SDL_CreateTexture(renderer, texture_format,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     new_size.cx, new_size.cy);
  if (t == nullptr)
    return;

  if (texture != nullptr)
      SDL_DestroyTexture(texture);
  texture = t;

#ifdef GREYSCALE
  buffer.Free();
  buffer.Allocate(new_size.cx, new_size.cy);
#endif
}

#endif

#ifdef GREYSCALE

#if CLANG_OR_GCC_VERSION(4,8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#endif

static void
CopyFromGreyscale(
#ifdef DITHER
                  Dither &dither,
#endif
                  SDL_Texture *dest,
                  ConstImageBuffer<GreyscalePixelTraits> src)
{
  uint8_t *dest_pixels;
  int pitch_as_int, dest_with, dest_height;
  SDL_QueryTexture(dest, nullptr, nullptr, &dest_with, &dest_height);
  if (SDL_LockTexture(dest, nullptr,
                      reinterpret_cast<void**>(&dest_pixels),
                      &pitch_as_int) != 0)
    return;

  int bytes_per_pixel = pitch_as_int / dest_with;

  assert(bytes_per_pixel == 4 || bytes_per_pixel == 2);

  const uint8_t *src_pixels = reinterpret_cast<const uint8_t *>(src.data);

  const unsigned width = src.width, height = src.height;

  const unsigned dest_pitch = (unsigned) pitch_as_int;

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

  ::SDL_UnlockTexture(dest);
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
#endif

  return Canvas(buffer);
}

void
TopCanvas::Unlock()
{
#ifndef GREYSCALE
  SDL_UnlockTexture(texture);
#endif
}

#endif

void
TopCanvas::Flip()
{
#ifdef ENABLE_OPENGL
  ::SDL_GL_SwapWindow(window);
#else

#ifdef GREYSCALE
  CopyFromGreyscale(
#ifdef DITHER
                    dither,
#endif
                    texture, buffer);
#endif

  ::SDL_RenderClear(renderer);
  ::SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  ::SDL_RenderPresent(renderer);

#endif
}
