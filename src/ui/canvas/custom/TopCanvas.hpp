/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_TOP_CANVAS_HPP
#define XCSOAR_SCREEN_TOP_CANVAS_HPP

#ifdef USE_MEMORY_CANVAS
#include "ui/canvas/memory/PixelTraits.hpp"
#include "ui/canvas/memory/ActivePixelTraits.hpp"
#include "ui/canvas/memory/Buffer.hpp"
#include "ui/dim/Size.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "ui/opengl/Features.hpp"
#endif

#ifdef USE_EGL
#include "ui/egl/System.hpp"

#ifdef MESA_KMS
#include "ui/canvas/egl/GbmSurface.hpp"
#include <xf86drm.h>
#include <xf86drmMode.h>
#endif
#endif

#ifdef USE_GLX
#include "ui/glx/System.hpp"
#endif

#ifdef DITHER
#include "../memory/Dither.hpp"
#endif

#include <cstdint>

#ifdef SOFTWARE_ROTATE_DISPLAY
enum class DisplayOrientation : uint8_t;
#endif

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
class Canvas;
struct PixelSize;
namespace UI { class Display; }

#if defined(USE_FB) && !defined(KOBO)
/* defined if we need to initialise /dev/tty to graphics mode, see
   TopCanvas::InitialiseTTY() */
#define USE_TTY
#include "ui/linux/GraphicsTTY.hpp"
#endif

class TopCanvas
{
  UI::Display &display;

#ifdef USE_TTY
  const LinuxGraphicsTTY linux_graphics_tty;
#endif

#ifdef USE_EGL

#ifdef MESA_KMS
  const EGL::GbmSurface gbm_surface;

  drmEventContext evctx;

  struct gbm_bo *current_bo = nullptr;

  drmModeCrtc *saved_crtc = nullptr;
#endif // MESA_KMS

  EGLSurface surface = EGL_NO_SURFACE;
#endif // USE_EGL

#ifdef USE_GLX
  GLXWindow glx_window;
#endif // USE_GLX

#ifdef ENABLE_SDL
  SDL_Window *const window;

#ifdef USE_MEMORY_CANVAS
  SDL_Renderer *renderer;
  SDL_Texture *texture;
#endif // USE_MEMORY_CANVAS
#endif // ENABLE_SDL

#ifdef USE_MEMORY_CANVAS

#ifdef GREYSCALE
  WritableImageBuffer<GreyscalePixelTraits> buffer;

#ifdef DITHER
  Dither dither;
#endif

#elif !defined(ENABLE_SDL)
  WritableImageBuffer<ActivePixelTraits> buffer;
#endif
#endif /* USE_MEMORY_CANVAS */

#ifdef USE_FB
  int fd = -1;

  void *map = nullptr;
  unsigned map_pitch, map_bpp;

  uint32_t epd_update_marker;
#endif // USE_FB

#ifdef KOBO
  /**
   * Runtime flag that can be used to disable dithering at runtime for
   * some situations.
   */
  bool enable_dither = true;

  /**
   * some kobo Device don't need to wait eInk update complet before send new update cmd
   * this flag can be set true for don't wait eInk Update complete for faster responce time.
   */
  bool frame_sync = false;
#endif // KOBO

public:
#ifdef ENABLE_SDL
  TopCanvas(UI::Display &_display, SDL_Window *_window);
#elif defined(USE_GLX)
  TopCanvas(UI::Display &_display,
            X11Window x_window);
#elif defined(USE_X11) || defined(USE_WAYLAND)
  TopCanvas(UI::Display &_display, EGLNativeWindowType native_window)
    :display(_display)
  {
    CreateSurface(native_window);
  }
#elif defined(USE_VFB)
  TopCanvas(UI::Display &_display, PixelSize new_size);
#else
  explicit TopCanvas(UI::Display &_display);
#endif

  ~TopCanvas() noexcept;

  /**
   * Is this object ready for drawing?
   */
  bool IsReady() const noexcept {
#ifdef USE_EGL
    /* can't draw if there is no EGL surface (e.g. if the Android app
       is paused) */
    return surface != EGL_NO_SURFACE;
#else
    return true;
#endif
  }

#if defined(USE_FB) || (defined(ENABLE_OPENGL) && (defined(USE_EGL) || defined(USE_GLX) || defined(ENABLE_SDL)))
  /**
   * Obtain the native (non-software-rotated) size of the OpenGL
   * drawable.
   */
  [[gnu::pure]]
  PixelSize GetNativeSize() const noexcept;
#endif

#if defined(USE_MEMORY_CANVAS) || defined(ENABLE_OPENGL)
  /**
   * Check if the screen has been resized.
   *
   * @param new_native_size the new screen size reported by the
   * windowing system library
   * @return true if the screen has been resized
   */
  bool CheckResize(PixelSize new_native_size) noexcept;
#endif

#ifdef USE_FB
  /**
   * Ask the kernel for the frame buffer's current physical size.
   * This is used by CheckResize().
   */
  [[gnu::pure]]
  PixelSize GetPhysicalSize() const noexcept;

  /**
   * Check if the screen has been resized.
   *
   * @return true if the screen has been resized
   */
  bool CheckResize() noexcept;
#endif

#ifdef ANDROID
  /**
   * Create an EGL surface.
   *
   * Throws on error.
   *
   * @return true on success, false if no surface is available
   * currently
   */
  bool AcquireSurface();
#endif

#ifdef USE_EGL
  void ReleaseSurface() noexcept;
#endif

#if defined(ENABLE_SDL) && defined(USE_MEMORY_CANVAS)
  void OnResize(PixelSize new_size) noexcept;
#endif

#if defined(USE_MEMORY_CANVAS) && (defined(GREYSCALE) || !defined(ENABLE_SDL))
  PixelSize GetSize() const noexcept {
    return PixelSize(buffer.width, buffer.height);
  }
#else
  [[gnu::pure]]
  PixelSize GetSize() const noexcept;
#endif

  Canvas Lock();
  void Unlock() noexcept;

  void Flip();

#ifdef KOBO
  /**
   * Wait until the screen update is complete.
   */
  void Wait() noexcept;

  void SetEnableDither(bool _enable_dither) noexcept {
    enable_dither = _enable_dither;
  }
#endif

#ifdef SOFTWARE_ROTATE_DISPLAY
  PixelSize SetDisplayOrientation(DisplayOrientation orientation) noexcept;
#endif

private:
#ifdef ENABLE_OPENGL
  PixelSize SetupViewport(PixelSize native_size) noexcept;
#endif

#ifdef USE_EGL
  void CreateSurface(EGLNativeWindowType native_window);
#endif
};

#endif
