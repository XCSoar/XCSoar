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
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/display/Display.hpp"
#include "ui/dim/Size.hpp"

#include <stdexcept>

TopCanvas::TopCanvas(UI::Display &_display,
                     X11Window x_window)
  :display(_display)
{
  glx_window = glXCreateWindow(display.GetXDisplay(), _display.GetFBConfig(),
                               x_window, nullptr);
  XSync(display.GetXDisplay(), false);

  if (!glXMakeContextCurrent(display.GetXDisplay(), glx_window, glx_window,
                             display.GetGLXContext()))
    throw std::runtime_error("Failed to attach GLX context to GLX window");

  const PixelSize effective_size = GetNativeSize();
  if (effective_size.width == 0 || effective_size.height == 0)
    throw std::runtime_error("Failed to query GLX drawable size");

  SetupViewport(effective_size);
}

TopCanvas::~TopCanvas() noexcept
{
  glXDestroyWindow(display.GetXDisplay(), glx_window);
}

PixelSize
TopCanvas::GetNativeSize() const noexcept
{
  unsigned w = 0, h = 0;
  glXQueryDrawable(display.GetXDisplay(), glx_window, GLX_WIDTH, &w);
  glXQueryDrawable(display.GetXDisplay(), glx_window, GLX_HEIGHT, &h);
  if (w <= 0 || h <= 0)
    return PixelSize(0, 0);

  return PixelSize(w, h);
}

void
TopCanvas::Flip()
{
  glXSwapBuffers(display.GetXDisplay(), glx_window);
}
