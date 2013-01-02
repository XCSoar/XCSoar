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

#include "Screen/Canvas.hpp"

#ifdef USE_EGL
#include "Screen/EGL/System.hpp"
#endif

class TopCanvas : public Canvas {
#ifdef USE_EGL
#ifdef USE_X11
  X11Window x_window;
#elif defined(USE_VIDEOCORE)
  /* for Raspberry Pi */
  DISPMANX_DISPLAY_HANDLE_T vc_display;
  DISPMANX_UPDATE_HANDLE_T vc_update;
  DISPMANX_ELEMENT_HANDLE_T vc_element;
  EGL_DISPMANX_WINDOW_T vc_window;
#endif

  EGLDisplay display;
  EGLContext context;
  EGLSurface surface;
#endif

public:
#ifdef USE_EGL
  ~TopCanvas();
#endif

  void Create(UPixelScalar width, UPixelScalar height,
              bool full_screen, bool resizable);

#ifdef ENABLE_OPENGL
  /**
   * Initialise the new OpenGL context.
   */
  void Resume();
#endif

  void OnResize(UPixelScalar width, UPixelScalar height);

#if defined(ANDROID) || defined(USE_EGL)
  void Fullscreen() {}
#else
  void Fullscreen();
#endif

  void Flip();
};

#endif
