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
#include "ui/dim/Size.hpp"

#ifdef USE_EGL
#include "ui/egl/System.hpp"
#endif

#ifdef USE_GLX
#include "ui/glx/System.hpp"
#include "LogFile.hpp"
#endif

#include <stdexcept>

namespace X11 {

#ifdef USE_GLX

[[gnu::pure]]
static int
GetConfigAttrib(_XDisplay *display, GLXFBConfig config,
                int attribute, int default_value) noexcept
{
  int value;
  return glXGetFBConfigAttrib(display, config, attribute, &value) == Success
    ? value
    : default_value;
}

#endif

Display::Display()
  :display(XOpenDisplay(nullptr))
{
  if (display == nullptr)
    throw std::runtime_error("XOpenDisplay() failed");

#ifdef USE_GLX
  const auto screen = DefaultScreen(display);

  static constexpr int attributes[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_X_RENDERABLE, true,
    GLX_DOUBLEBUFFER, true,
    GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1,
    GLX_ALPHA_SIZE, 1,
    GLX_STENCIL_SIZE, 1,
    0
  };

  int fb_cfg_count;
  fb_cfg = glXChooseFBConfig(display, screen,
                             attributes, &fb_cfg_count);
  if (fb_cfg == nullptr || fb_cfg_count == 0)
    throw std::runtime_error("Failed to retrieve framebuffer configuration for GLX");

  LogFormat("GLX config: RGB=%d/%d/%d alpha=%d depth=%d stencil=%d",
            GetConfigAttrib(display, *fb_cfg, GLX_RED_SIZE, 0),
            GetConfigAttrib(display, *fb_cfg, GLX_GREEN_SIZE, 0),
            GetConfigAttrib(display, *fb_cfg, GLX_BLUE_SIZE, 0),
            GetConfigAttrib(display, *fb_cfg, GLX_ALPHA_SIZE, 0),
            GetConfigAttrib(display, *fb_cfg, GLX_DEPTH_SIZE, 0),
            GetConfigAttrib(display, *fb_cfg, GLX_STENCIL_SIZE, 0));

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

PixelSize
Display::GetSize() const noexcept
{
  return {DisplayWidth(display, 0), DisplayHeight(display, 0)};
}

PixelSize
Display::GetSizeMM() const noexcept
{
  return {DisplayWidthMM(display, 0), DisplayHeightMM(display, 0)};
}

} // namespace X11
