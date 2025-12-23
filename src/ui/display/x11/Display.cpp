// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

Display::Display(unsigned antialiasing_samples)
  :display(XOpenDisplay(nullptr))
{
  if (display == nullptr)
    throw std::runtime_error("XOpenDisplay() failed");

#ifdef USE_GLX
  const auto screen = DefaultScreen(display);

  // Try with requested antialiasing first
  if (antialiasing_samples > 0) {
    int attributes[32];
    int i = 0;
    attributes[i++] = GLX_DRAWABLE_TYPE; attributes[i++] = GLX_WINDOW_BIT;
    attributes[i++] = GLX_RENDER_TYPE; attributes[i++] = GLX_RGBA_BIT;
    attributes[i++] = GLX_X_RENDERABLE; attributes[i++] = true;
    attributes[i++] = GLX_DOUBLEBUFFER; attributes[i++] = true;
    attributes[i++] = GLX_RED_SIZE; attributes[i++] = 1;
    attributes[i++] = GLX_GREEN_SIZE; attributes[i++] = 1;
    attributes[i++] = GLX_BLUE_SIZE; attributes[i++] = 1;
    attributes[i++] = GLX_ALPHA_SIZE; attributes[i++] = 1;
    attributes[i++] = GLX_STENCIL_SIZE; attributes[i++] = 1;
    attributes[i++] = GLX_SAMPLE_BUFFERS; attributes[i++] = 1;
    attributes[i++] = GLX_SAMPLES; attributes[i++] = antialiasing_samples;
    attributes[i++] = 0;

    int fb_cfg_count;
    fb_cfg = glXChooseFBConfig(display, screen, attributes, &fb_cfg_count);
    
    if (fb_cfg != nullptr && fb_cfg_count > 0) {
      LogFormat("GLX config: RGB=%d/%d/%d alpha=%d depth=%d stencil=%d samples=%d",
                GetConfigAttrib(display, *fb_cfg, GLX_RED_SIZE, 0),
                GetConfigAttrib(display, *fb_cfg, GLX_GREEN_SIZE, 0),
                GetConfigAttrib(display, *fb_cfg, GLX_BLUE_SIZE, 0),
                GetConfigAttrib(display, *fb_cfg, GLX_ALPHA_SIZE, 0),
                GetConfigAttrib(display, *fb_cfg, GLX_DEPTH_SIZE, 0),
                GetConfigAttrib(display, *fb_cfg, GLX_STENCIL_SIZE, 0),
                GetConfigAttrib(display, *fb_cfg, GLX_SAMPLES, 0));

      glx_context = glXCreateNewContext(display, *fb_cfg,
                                        GLX_RGBA_TYPE,
                                        nullptr, true);
      if (glx_context == nullptr)
        throw std::runtime_error("Failed to create GLX context");

      if (!glXMakeContextCurrent(display, 0, 0, glx_context))
        throw std::runtime_error("Failed to enable GLX context");
      
      return;
    }

    LogFormat("Requested %ux anti-aliasing not available, disabling",
              antialiasing_samples);
  }

  // Fallback: no antialiasing
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
