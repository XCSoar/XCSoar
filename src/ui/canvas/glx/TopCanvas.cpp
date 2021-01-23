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

#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/opengl/Init.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/opengl/Features.hpp"

#include <stdexcept>

void
TopCanvas::CreateGLX(_XDisplay *_x_display,
                     X11Window x_window,
                     GLXFBConfig *fb_cfg)
{
  x_display = _x_display;

  glx_context = glXCreateNewContext(_x_display, *fb_cfg, GLX_RGBA_TYPE,
                                    nullptr, true);
  if (glx_context == nullptr)
    throw std::runtime_error("Failed to create GLX context");

  glx_window = glXCreateWindow(_x_display, *fb_cfg, x_window, nullptr);
  XSync(x_display, false);

  if (!glXMakeContextCurrent(_x_display, glx_window, glx_window, glx_context))
    throw std::runtime_error("Failed to attach GLX context to GLX window");

  const PixelSize effective_size = GetNativeSize();
  if (effective_size.width == 0 || effective_size.height == 0)
    throw std::runtime_error("Failed to query GLX drawable size");

  OpenGL::SetupContext();
  SetupViewport(effective_size);
}

void
TopCanvas::Destroy()
{
  glXDestroyWindow(x_display, glx_window);
  glXDestroyContext(x_display, glx_context);
}

PixelSize
TopCanvas::GetNativeSize() const
{
  unsigned w = 0, h = 0;
  glXQueryDrawable(x_display, glx_window, GLX_WIDTH, &w);
  glXQueryDrawable(x_display, glx_window, GLX_HEIGHT, &h);
  if (w <= 0 || h <= 0)
    return PixelSize(0, 0);

  return PixelSize(w, h);
}

void
TopCanvas::Flip()
{
  glXSwapBuffers(x_display, glx_window);
}
