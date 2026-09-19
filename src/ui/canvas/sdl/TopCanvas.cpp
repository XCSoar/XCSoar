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

#include <SDL3/SDL_platform.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_hints.h>
#ifdef USE_MEMORY_CANVAS
#include <SDL3/SDL_render.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif

#include <cassert>

#ifdef ENABLE_OPENGL

[[gnu::pure]]
static int
GetConfigAttrib(SDL_GLAttr attribute, int default_value) noexcept
{
  int value;
  return SDL_GL_GetAttribute(attribute, &value)
    ? value
    : default_value;
}

#endif

TopCanvas::TopCanvas(UI::Display &_display, SDL_Window *_window)
  :display(_display), window(_window)
{
#ifdef USE_MEMORY_CANVAS
  renderer = SDL_CreateRenderer(window, nullptr);
  if (renderer == nullptr)
    throw FmtRuntimeError("SDL_CreateRenderer({}) has failed: {}",
                          (const void *)window, ::SDL_GetError());

  int width = 0, height = 0;
  if (!SDL_GetCurrentRenderOutputSize(renderer, &width, &height)) {
    SDL_DestroyRenderer(renderer);
    throw FmtRuntimeError("SDL_GetCurrentRenderOutputSize() failed: {}",
                          SDL_GetError());
  }
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                              SDL_TEXTUREACCESS_STREAMING,
                              width, height);
  if (texture == nullptr) {
    SDL_DestroyRenderer(renderer);
    throw FmtRuntimeError("SDL_CreateTexture({}, {}, {}, {}, {}) has failed: {}",
                          (const void *)renderer,
                          (unsigned)SDL_PIXELFORMAT_UNKNOWN,
                          (unsigned)SDL_TEXTUREACCESS_STREAMING,
                          width, height,
                          ::SDL_GetError());
  }
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
  SDL_DestroyRenderer(renderer);
#endif
}

#ifdef ENABLE_OPENGL

PixelSize
TopCanvas::GetNativeSize() const noexcept
{
  int w = 0, h = 0;
  SDL_GetWindowSizeInPixels(window, &w, &h);
  return PixelSize(w, h);
}

#endif

#ifdef USE_MEMORY_CANVAS

#ifndef GREYSCALE

PixelSize
TopCanvas::GetSize() const noexcept
{
  float width = 0, height = 0;
  if (!SDL_GetTextureSize(texture, &width, &height))
    return {};

  return PixelSize(unsigned(width), unsigned(height));
}

#endif // !GREYSCALE

void
TopCanvas::OnResize(PixelSize new_size) noexcept
{
  if (new_size == GetSize())
    return;

  SDL_Texture *t = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
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

void
TopCanvas::RequestResize(PixelSize new_size) noexcept
{
  // Store size components atomically
  // Note: We store width/height separately to avoid torn reads
  pending_width.store(new_size.width, std::memory_order_relaxed);
  pending_height.store(new_size.height, std::memory_order_relaxed);

  // Set flag last with release semantics to ensure size writes are visible
  resize_pending.store(true, std::memory_order_release);
}

bool
TopCanvas::ProcessPendingResize() noexcept
{
  // Check and clear flag atomically with acquire semantics
  if (!resize_pending.exchange(false, std::memory_order_acquire))
    return false;

  // Read the size that was written before the flag was set
  const PixelSize new_size{
    pending_width.load(std::memory_order_relaxed),
    pending_height.load(std::memory_order_relaxed)
  };

  // Perform the actual resize in the UI thread
  OnResize(new_size);

  return true;
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
  float dest_width = 0;
  float ignored_height = 0;
  if (!SDL_GetTextureSize(dest, &dest_width, &ignored_height) ||
      dest_width <= 0)
    return;

  int pitch_as_int;
  if (!SDL_LockTexture(dest, nullptr,
                       reinterpret_cast<void **>(&dest_pixels),
                       &pitch_as_int))
    return;

  int bytes_per_pixel = pitch_as_int / int(dest_width);

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
  int pitch;
  if (!SDL_LockTexture(texture, nullptr, &pixels, &pitch))
    return Canvas();
  buffer.data = (ActivePixelTraits::pointer)pixels;
  buffer.pitch = (unsigned) pitch;
  buffer.size = GetSize();
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

  ::SDL_RenderTexture(renderer, texture, nullptr, nullptr);
  ::SDL_RenderPresent(renderer);

#endif
}

#if defined(__APPLE__) && TARGET_OS_IPHONE

bool
TopCanvas::IsIOSAppActive() const noexcept
{
  // Check if the iOS app is in an active state where rendering is appropriate
  // Attempting to render while the app is in the background will crash the app
  UIApplicationState appState = [[UIApplication sharedApplication] applicationState];
  
  switch (appState) {
    case UIApplicationStateActive:
      // App is active and in foreground - safe to render
      return true;
      
    case UIApplicationStateInactive:
      // App is transitioning between states - avoid rendering
      return false;
      
    case UIApplicationStateBackground:
      // App is in background - definitely don't render
      return false;
      
    default:
      // Unknown state - we are conservative and don't render
      return false;
  }
}

#endif
