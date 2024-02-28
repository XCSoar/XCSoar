// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/Features.hpp"
#include "ui/dim/Size.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "ui/dim/Rect.hpp"
#include "ui/canvas/opengl/Init.hpp"
#include "Math/Point2D.hpp"
#include "LogFile.hpp"
#else
#include "ui/canvas/memory/Export.hpp"
#include "ui/canvas/Canvas.hpp"
#endif

#ifdef DITHER
#include "ui/canvas/memory/Dither.hpp"
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

#include <cassert>

#ifdef ENABLE_OPENGL

[[gnu::pure]]
static int
GetConfigAttrib(SDL_GLattr attribute, int default_value) noexcept
{
  int value;
  return SDL_GL_GetAttribute(attribute, &value) == 0
    ? value
    : default_value;
}

#endif

TopCanvas::TopCanvas(UI::Display &_display, SDL_Window *_window)
  :display(_display), window(_window)
{
#ifdef USE_MEMORY_CANVAS
  renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == nullptr)
    throw FmtRuntimeError("SDL_CreateRenderer({}, {}, {}) has failed: {}",
                          (const void *)window, -1, 0, ::SDL_GetError());

  int width, height;
  SDL_GetRendererOutputSize(renderer, &width, &height);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                              SDL_TEXTUREACCESS_STREAMING,
                              width, height);
  if (texture == nullptr)
    throw FmtRuntimeError("SDL_CreateTexture({}, {}, {}, {}, {}) has failed: {}",
                          (const void *)renderer,
                          (unsigned)SDL_PIXELFORMAT_UNKNOWN,
                          (unsigned)SDL_TEXTUREACCESS_STREAMING,
                          width, height,
                          ::SDL_GetError());
#endif

#ifdef ENABLE_OPENGL
  if (::SDL_GL_CreateContext(window) == nullptr)
    throw FmtRuntimeError("SDL_GL_CreateContext({}) has failed: {}",
                          (const void *)window, ::SDL_GetError());

  LogFormat("SDL_GL config: RGB=%d/%d/%d alpha=%d depth=%d stencil=%d",
            GetConfigAttrib(SDL_GL_RED_SIZE, 0),
            GetConfigAttrib(SDL_GL_GREEN_SIZE, 0),
            GetConfigAttrib(SDL_GL_BLUE_SIZE, 0),
            GetConfigAttrib(SDL_GL_ALPHA_SIZE, 0),
            GetConfigAttrib(SDL_GL_DEPTH_SIZE, 0),
            GetConfigAttrib(SDL_GL_STENCIL_SIZE, 0));

  /* this is usually done by OpenGL::Display, but libSDL doesn't allow
     that */
  OpenGL::SetupContext();

  SetupViewport(GetNativeSize());
#endif

#ifdef GREYSCALE
  buffer.Allocate(PixelSize(width, height));
#endif
}

TopCanvas::~TopCanvas() noexcept
{
#if !defined(ENABLE_OPENGL) && defined(GREYSCALE)
  buffer.Free();
#endif

#ifdef USE_MEMORY_CANVAS
  SDL_DestroyTexture(texture);
#endif
}

#ifdef ENABLE_OPENGL

PixelSize
TopCanvas::GetNativeSize() const noexcept
{
  int w, h;
  SDL_GL_GetDrawableSize(window, &w, &h);
  return PixelSize(w, h);
}

#endif

#ifdef USE_MEMORY_CANVAS

#ifndef GREYSCALE

PixelSize
TopCanvas::GetSize() const noexcept
{
  int width, height;
  if (SDL_QueryTexture(texture, nullptr, nullptr, &width, &height) != 0)
    return {};

  return PixelSize(width, height);
}

#endif // !GREYSCALE

void
TopCanvas::OnResize(PixelSize new_size) noexcept
{
  int texture_width, texture_height;
  Uint32 texture_format;
  if (SDL_QueryTexture(texture, &texture_format, NULL, &texture_width, &texture_height) != 0)
    return;
  if ((int)new_size.width == texture_width && (int)new_size.height == texture_height)
    return;

  SDL_Texture *t = SDL_CreateTexture(renderer, texture_format,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     new_size.width, new_size.height);
  if (t == nullptr)
    return;

  if (texture != nullptr)
      SDL_DestroyTexture(texture);
  texture = t;

#ifdef GREYSCALE
  buffer.Free();
  buffer.Allocate(new_size);
#endif
}

#endif // USE_MEMORY_CANVAS

#ifdef GREYSCALE

#ifdef __GNUC__
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

  const unsigned dest_pitch = (unsigned) pitch_as_int;

#ifdef DITHER

  dither.DitherGreyscale(src_pixels, src.pitch,
                         dest_pixels,
                         dest_pitch / bytes_per_pixel,
                         src.size.width, src.size.height);
  if (bytes_per_pixel == 4) {
    const unsigned n_pixels = (dest_pitch / bytes_per_pixel)
      * src.size.height;
    int32_t *d = (int32_t *)dest_pixels + n_pixels;
    const int8_t *end = (int8_t *)dest_pixels;
    const int8_t *s = end + n_pixels;

    while (s != end)
      *--d = *--s;
  }

#else

  const unsigned src_pitch = src.pitch;

  if (bytes_per_pixel == 2) {
    for (unsigned row = src.size.height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      CopyGreyscaleToRGB565((RGB565Color *)dest_pixels,
                            (const Luminosity8 *)src_pixels, src.size.width);
  } else {
    for (unsigned row = src.size.height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      CopyGreyscaleToRGB8((uint32_t *)dest_pixels,
                           (const Luminosity8 *)src_pixels, src.size.width);
  }

#endif

  ::SDL_UnlockTexture(dest);
}

#ifdef __GNUC__
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
  buffer.data = (ActivePixelTraits::pointer)pixels;
  buffer.pitch = (unsigned) pitch;
  buffer.size = PixelSize(width, height);
#endif

  return Canvas(buffer);
}

void
TopCanvas::Unlock() noexcept
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

  ::SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  ::SDL_RenderPresent(renderer);

#endif
}
