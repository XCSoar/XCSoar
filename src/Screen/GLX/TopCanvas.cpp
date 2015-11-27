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
#include "Screen/OpenGL/Init.hpp"
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/OpenGL/Features.hpp"

#include <stdio.h>

void
TopCanvas::CreateGLX(_XDisplay *_x_display,
                     X11Window x_window,
                     GLXFBConfig *fb_cfg)
{
  x_display = _x_display;

  glx_context = glXCreateNewContext(_x_display, *fb_cfg, GLX_RGBA_TYPE,
                                    nullptr, true);
  if (glx_context == nullptr) {
    fprintf(stderr, "Failed to create GLX context\n");
    exit(EXIT_FAILURE);
  }
  glx_window = glXCreateWindow(_x_display, *fb_cfg, x_window, nullptr);
  XSync(x_display, false);

  if (!glXMakeContextCurrent(_x_display, glx_window, glx_window, glx_context)) {
    fprintf(stderr, "Failed to attach GLX context to GLX window\n");
    exit(EXIT_FAILURE);
  }

  unsigned int glx_width = -1, glx_height = -1;
  glXQueryDrawable(_x_display, glx_window, GLX_WIDTH, &glx_width);
  glXQueryDrawable(_x_display, glx_window, GLX_HEIGHT, &glx_height);
  if ((glx_width <= 0) || (glx_height <= 0)) {
    fprintf(stderr, "Failed to query GLX drawable size\n");
    exit(EXIT_FAILURE);
  }
  const PixelSize effective_size = { glx_width, glx_height };

  OpenGL::SetupContext();
  OpenGL::SetupViewport(UnsignedPoint2D(effective_size.cx,
                                        effective_size.cy));
  Canvas::Create(effective_size);
}

void
TopCanvas::Destroy()
{
  glXDestroyWindow(x_display, glx_window);
  glXDestroyContext(x_display, glx_context);
}

void
TopCanvas::OnResize(PixelSize new_size)
{
  if (new_size == size)
    return;

  OpenGL::SetupViewport(UnsignedPoint2D(new_size.cx, new_size.cy));
  Canvas::Create(new_size);
}

void
TopCanvas::Flip()
{
  glXSwapBuffers(x_display, glx_window);
}
