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
#include "Screen/OpenGL/Init.hpp"
#include "Screen/OpenGL/EGL.hpp"
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/OpenGL/Features.hpp"

#include <stdio.h>
#include <stdlib.h>

void
TopCanvas::Create(PixelSize new_size,
                  bool full_screen, bool resizable)
{
#ifdef USE_X11
  X11Display *const x_display = XOpenDisplay(nullptr);
  if (x_display == nullptr) {
    fprintf(stderr, "XOpenDisplay() failed\n");
    exit(EXIT_FAILURE);
  }

  const X11Window x_root = DefaultRootWindow(x_display);
  if (x_root == 0) {
    fprintf(stderr, "DefaultRootWindow() failed\n");
    exit(EXIT_FAILURE);
  }

  XSetWindowAttributes swa;
  swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask;

  x_window = XCreateWindow(x_display, x_root,
                           0, 0, 640, 480, 0,
                           CopyFromParent, InputOutput,
                           CopyFromParent, CWEventMask,
                           &swa);
  if (x_window == 0) {
    fprintf(stderr, "XCreateWindow() failed\n");
    exit(EXIT_FAILURE);
  }

  XMapWindow(x_display, x_window);
  XStoreName(x_display, x_window, "XCSoar");

  const EGLNativeDisplayType native_display = x_display;
  const EGLNativeWindowType native_window = x_window;
#elif defined(USE_VIDEOCORE)
  vc_display = vc_dispmanx_display_open(0);
  vc_update = vc_dispmanx_update_start(0);

  VC_RECT_T dst_rect;
  dst_rect.x = dst_rect.y = 0;
  dst_rect.width = new_size.cx;
  dst_rect.height = new_size.cy;

  VC_RECT_T src_rect = dst_rect;
  src_rect.x = src_rect.y = 0;
  src_rect.width = new_size.cx << 16;
  src_rect.height = new_size.cy << 16;

  vc_element = vc_dispmanx_element_add(vc_update, vc_display,
                                       0, &dst_rect, 0, &src_rect,
                                       DISPMANX_PROTECTION_NONE,
                                       0, 0,
                                       DISPMANX_NO_ROTATE);
  vc_dispmanx_update_submit_sync(vc_update);

  vc_window.element = vc_element;
  vc_window.width = new_size.cx;
  vc_window.height = new_size.cy;

  const EGLNativeDisplayType native_display = EGL_DEFAULT_DISPLAY;
  const EGLNativeWindowType native_window = &vc_window;
#elif defined(HAVE_MALI)
  const EGLNativeDisplayType native_display = EGL_DEFAULT_DISPLAY;
  mali_native_window.width = new_size.cx;
  mali_native_window.height = new_size.cy;
  struct mali_native_window *native_window = &mali_native_window;
#endif

  display = eglGetDisplay(native_display);
  if (display == EGL_NO_DISPLAY) {
    fprintf(stderr, "eglGetDisplay(EGL_DEFAULT_DISPLAY) failed\n");
    exit(EXIT_FAILURE);
  }

  if (!eglInitialize(display, NULL, NULL)) {
    fprintf(stderr, "eglInitialize() failed\n");
    exit(EXIT_FAILURE);
  }

  if (!eglBindAPI(HaveGLES() ? EGL_OPENGL_ES_API : EGL_OPENGL_API)) {
    fprintf(stderr, "eglBindAPI() failed\n");
    exit(EXIT_FAILURE);
  }

  static constexpr EGLint attributes[] = {
    EGL_STENCIL_SIZE, 1,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, HaveGLES() ? EGL_OPENGL_ES_BIT : EGL_OPENGL_BIT,
    EGL_NONE
  };

  EGLint num_configs;
  EGLConfig chosen_config = 0;
  eglChooseConfig(display, attributes, &chosen_config, 1, &num_configs);
  if (num_configs == 0) {
    fprintf(stderr, "eglChooseConfig() failed\n");
    exit(EXIT_FAILURE);
  }

  surface = eglCreateWindowSurface(display, chosen_config,
                                   native_window, nullptr);
  if (surface == nullptr) {
    fprintf(stderr, "eglCreateWindowSurface() failed\n");
    exit(EXIT_FAILURE);
  }

  GLint egl_width, egl_height;
  if (!eglQuerySurface(display, surface, EGL_WIDTH, &egl_width) ||
      !eglQuerySurface(display, surface, EGL_HEIGHT, &egl_height)) {
    fprintf(stderr, "eglQuerySurface()\n");
    exit(EXIT_FAILURE);
  }

  const PixelSize effective_size = { egl_width, egl_height };

  context = eglCreateContext(display, chosen_config,
                             EGL_NO_CONTEXT, nullptr);

  eglMakeCurrent(display, surface, surface, context);

  OpenGL::SetupContext();
  OpenGL::SetupViewport(effective_size.cx, effective_size.cy);
  Canvas::Create(effective_size);
}

void
TopCanvas::Destroy()
{
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(display, surface);
  eglDestroyContext(display, context);
  eglTerminate(display);
}

void
TopCanvas::OnResize(PixelSize new_size)
{
  if (new_size == size)
    return;

  OpenGL::SetupViewport(new_size.cx, new_size.cy);
  Canvas::Create(new_size);
}

void
TopCanvas::Flip()
{
  eglSwapBuffers(display, surface);
}
