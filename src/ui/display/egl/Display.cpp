// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "ConfigChooser.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "LogFile.hpp"

#include <cassert>

namespace EGL {

Display::Display(EGLNativeDisplayType native_display,
                 unsigned antialiasing_samples)
{
  InitDisplay(native_display, antialiasing_samples);
  CreateContext();
}

Display::~Display() noexcept
{
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  if (dummy_surface != EGL_NO_SURFACE)
    eglDestroySurface(display, dummy_surface);

  eglDestroyContext(display, context);
  eglTerminate(display);
}

[[gnu::pure]]
static int
GetConfigAttrib(EGLDisplay display, EGLConfig config,
                int attribute, int default_value) noexcept
{
  int value;
  return eglGetConfigAttrib(display, config, attribute, &value)
    ? value
    : default_value;
}

inline void
Display::InitDisplay(EGLNativeDisplayType native_display,
                     unsigned requested_antialiasing_samples)
{
  assert(display == EGL_NO_DISPLAY);

  /* remember what the profile asked for: a framebuffer object can
     still provide it even if the window surface configuration below
     falls back to fewer samples (or none) */
  OpenGL::requested_antialiasing_samples = requested_antialiasing_samples;

  display = eglGetDisplay(native_display);
  if (display == EGL_NO_DISPLAY)
    throw std::runtime_error("eglGetDisplay(EGL_DEFAULT_DISPLAY) failed");

  if (!eglInitialize(display, nullptr, nullptr))
    throw std::runtime_error("eglInitialize() failed");

  if (const char *s = eglQueryString(display, EGL_VENDOR))
    LogFormat("EGL vendor: %s", s);

  if (const char *s = eglQueryString(display, EGL_VERSION))
    LogFormat("EGL version: %s", s);

  if (const char *s = eglQueryString(display, EGL_EXTENSIONS))
    LogFormat("EGL extensions: %s", s);

  if (!eglBindAPI(EGL_OPENGL_ES_API))
    throw std::runtime_error("eglBindAPI() failed");

  /* this probes the display and publishes
     OpenGL::available_antialiasing_samples as a side effect, before
     picking the config; see its implementation for why */
  chosen_config = EGL::ChooseConfig(display, requested_antialiasing_samples);

  const unsigned samples = GetConfigAttrib(display, chosen_config,
                                           EGL_SAMPLES, 0);

  LogFormat("EGL config: RGB=%d/%d/%d alpha=%d depth=%d stencil=%d samples=%u"
            " (requested %u)",
            GetConfigAttrib(display, chosen_config, EGL_RED_SIZE, 0),
            GetConfigAttrib(display, chosen_config, EGL_GREEN_SIZE, 0),
            GetConfigAttrib(display, chosen_config, EGL_BLUE_SIZE, 0),
            GetConfigAttrib(display, chosen_config, EGL_ALPHA_SIZE, 0),
            GetConfigAttrib(display, chosen_config, EGL_DEPTH_SIZE, 0),
            GetConfigAttrib(display, chosen_config, EGL_STENCIL_SIZE, 0),
            samples, requested_antialiasing_samples);

  OpenGL::SetAntialiasingSamples(samples);
}

inline void
Display::CreateContext()
{
  assert(display != EGL_NO_DISPLAY);
  assert(context == EGL_NO_CONTEXT);

  static constexpr EGLint context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  context = eglCreateContext(display, chosen_config,
                             EGL_NO_CONTEXT, context_attributes);
  if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
    /* some old EGL implemenations do not support EGL_NO_SURFACE
       (error EGL_BAD_MATCH); this kludge uses a dummy 1x1 pbuffer
       surface to work around this */

    static constexpr int pbuffer_attributes[] = {
      EGL_WIDTH, 1,
      EGL_HEIGHT, 1,
      EGL_NONE
    };

    dummy_surface = eglCreatePbufferSurface(display, chosen_config,
                                            pbuffer_attributes);
    if (dummy_surface == EGL_NO_SURFACE)
      throw FmtRuntimeError("eglCreatePbufferSurface() failed: {:#x}", eglGetError());

    MakeCurrent(dummy_surface);
  }
}

EGLSurface
Display::CreateWindowSurface(EGLNativeWindowType native_window)
{
  auto surface = eglCreateWindowSurface(display, chosen_config,
                                        native_window, nullptr);
  if (surface == EGL_NO_SURFACE)
    throw FmtRuntimeError("eglCreateWindowSurface() failed: {:#x}", eglGetError());

  return surface;
}

void
Display::MakeCurrent(EGLSurface surface)
{
  if (surface == EGL_NO_SURFACE)
    surface = dummy_surface;

  if (!eglMakeCurrent(display, surface, surface, context))
    throw FmtRuntimeError("eglMakeCurrent() failed: {:#x}", eglGetError());

#ifdef USE_WAYLAND
  /* Mesa waits in eglSwapBuffers for a compositor frame callback.
     Hidden surfaces never get one, which stalls the UI thread.
     Interval 0 draws without that wait.  eglSwapInterval applies to
     the current draw surface, so this must run after MakeCurrent.
     Refresh() still skips Flip() while the surface is SUSPENDED. */
  if (!eglSwapInterval(display, 0))
    LogFormat("eglSwapInterval(0) failed: %#x", eglGetError());
#endif
}

} // namespace EGL
