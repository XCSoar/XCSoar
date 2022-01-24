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

#include "Display.hpp"
#include "Hardware/DisplayDPI.hpp"

#ifdef USE_EGL
#include "ui/egl/System.hpp"
#endif

#ifdef USE_GLX
#include "ui/glx/System.hpp"
#endif

#include <cassert>
#include <stdexcept>

namespace X11 {

Display::Display()
  :display(XOpenDisplay(nullptr))
{
  if (display == nullptr)
    throw std::runtime_error("XOpenDisplay() failed");

  /* query the display dimensions from Xlib to calculate the DPI
     value */
  const auto screen = DefaultScreen(display);
  const auto width_pixels = DisplayWidth(display, screen);
  const auto height_pixels = DisplayHeight(display, screen);
  const auto width_mm = DisplayWidthMM(display, screen);
  const auto height_mm = DisplayHeightMM(display, screen);
  if (width_pixels > 0 && height_pixels > 0 &&
      width_mm > 0 && height_mm > 0)
    ::Display::ProvideSizeMM(width_pixels, height_pixels,
                             width_mm, height_mm);

#ifdef USE_GLX
  static constexpr int attributes[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_X_RENDERABLE, true,
    GLX_DOUBLEBUFFER, true,
    GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1,
    GLX_ALPHA_SIZE, 1,
    0
  };

  int fb_cfg_count;
  fb_cfg = glXChooseFBConfig(display, screen,
                             attributes, &fb_cfg_count);
  if (fb_cfg == nullptr || fb_cfg_count == 0)
    throw std::runtime_error("Failed to retrieve framebuffer configuration for GLX");

  glx_context = glXCreateNewContext(display, *fb_cfg,
                                    GLX_RGBA_TYPE,
                                    nullptr, true);
  if (glx_context == nullptr)
    throw std::runtime_error("Failed to create GLX context");

  if (!glXMakeContextCurrent(display, 0, 0, glx_context))
    throw std::runtime_error("Failed to enable GLX context");
#endif // USE_GLX
}

Display::~Display() noexcept
{
#ifdef USE_GLX
  glXDestroyContext(display, glx_context);
#endif // USE_GLX

  XCloseDisplay(display);
}

} // namespace X11
