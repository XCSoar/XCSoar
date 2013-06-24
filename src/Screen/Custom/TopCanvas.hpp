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

#ifndef XCSOAR_SCREEN_TOP_CANVAS_HPP
#define XCSOAR_SCREEN_TOP_CANVAS_HPP

#include "Compiler.h"

#ifndef USE_MEMORY_CANVAS
#include "Screen/Canvas.hpp"
#elif defined(GREYSCALE)
#include "Screen/Memory/PixelTraits.hpp"
#include "Screen/Memory/Buffer.hpp"
#endif

#ifdef USE_EGL
#include "Screen/EGL/System.hpp"
#endif

#ifdef DITHER
#include "Dither.hpp"
#endif

struct SDL_Surface;
class Canvas;
struct PixelSize;
struct PixelRect;

class TopCanvas
#ifndef USE_MEMORY_CANVAS
  : public Canvas
#endif
{
#ifdef USE_EGL
#ifdef USE_X11
  X11Window x_window;
#elif defined(USE_VIDEOCORE)
  /* for Raspberry Pi */
  DISPMANX_DISPLAY_HANDLE_T vc_display;
  DISPMANX_UPDATE_HANDLE_T vc_update;
  DISPMANX_ELEMENT_HANDLE_T vc_element;
  EGL_DISPMANX_WINDOW_T vc_window;
#elif defined(HAVE_MALI)
  struct mali_native_window mali_native_window;
#endif

  EGLDisplay display;
  EGLContext context;
  EGLSurface surface;
#endif

#ifdef USE_MEMORY_CANVAS
#ifdef ENABLE_SDL
  SDL_Surface *surface;
#endif

#ifdef GREYSCALE
  WritableImageBuffer<GreyscalePixelTraits> buffer;

#ifdef DITHER
  Dither dither;
#endif
#endif
#endif

#ifdef USE_FB
  int fd;

  void *map;
  unsigned map_pitch, map_bpp;

  uint32_t epd_update_marker;
#endif

public:
#ifdef USE_FB
  TopCanvas():fd(-1), map(nullptr) {}
  ~TopCanvas() {
    Destroy();
  }

  void Destroy();
#endif

#if defined(USE_EGL) || (defined(USE_MEMORY_CANVAS) && defined(GREYSCALE) && !defined(USE_FB))
  ~TopCanvas();
#endif

#ifdef USE_MEMORY_CANVAS
  bool IsDefined() const {
#ifdef ENABLE_SDL
    return surface != nullptr;
#else
    return fd >= 0;
#endif
  }

  gcc_pure
  PixelRect GetRect() const;
#endif

  void Create(PixelSize new_size,
              bool full_screen, bool resizable);

#ifdef ENABLE_OPENGL
  /**
   * Initialise the new OpenGL context.
   */
  void Resume();
#endif

  void OnResize(PixelSize new_size);

#if defined(ANDROID) || defined(USE_EGL)
  void Fullscreen() {}
#else
  void Fullscreen();
#endif

#ifdef USE_MEMORY_CANVAS
  Canvas Lock();
  void Unlock();
#endif

  void Flip();
};

#endif
