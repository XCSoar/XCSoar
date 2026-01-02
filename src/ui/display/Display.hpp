// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_X11
#include "x11/Display.hpp"
#endif

#ifdef USE_WAYLAND
#include "wayland/Display.hpp"
#endif

#ifdef MESA_KMS
#include "egl/DrmDisplay.hpp"
#include "egl/GbmDisplay.hpp"
#endif

#ifdef USE_EGL
#include "egl/Display.hpp"
#endif

#ifdef ENABLE_SDL
#include "sdl/Display.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "opengl/Display.hpp"
#endif

#ifdef USE_GDI
#include "gdi/Display.hpp"
#endif

namespace UI {

/**
 * The #Display establishes a connection to the native display and
 * gives the rest of the UI toolkit unified access to it.
 */

#ifdef ANDROID

class Display : public EGL::Display, public OpenGL::Display {
public:
  explicit Display(EGLNativeDisplayType native_display,
                   unsigned antialiasing_samples = 0)
    :EGL::Display(native_display, antialiasing_samples),
     OpenGL::Display(antialiasing_samples) {}
};

#elif defined(USE_EGL) && defined(USE_X11)

class Display
  : public X11::Display, public EGL::Display,
    public OpenGL::Display {
public:
  explicit Display(unsigned antialiasing_samples = 0)
    :X11::Display(antialiasing_samples),
     EGL::Display(X11::Display::GetXDisplay()),
     OpenGL::Display(antialiasing_samples) {}
};

#elif defined(USE_GLX) && defined(USE_X11)

class Display : public X11::Display, public OpenGL::Display {
public:
  explicit Display(unsigned antialiasing_samples = 0)
    :X11::Display(antialiasing_samples),
     OpenGL::Display(antialiasing_samples) {}
};

#elif defined(MESA_KMS)

class Display
  : public EGL::DrmDisplay, public EGL::GbmDisplay, public EGL::Display,
    public OpenGL::Display
{
  /* for DRM/GBM/KMS, we first need to open a DRM device
     (#DrmDisplay), allocate a GBM display (#GbmDisplay) and then
     initialise EGL (#EGL::Display) */

  /**
   * Are the screen contents "diry" and a redraw is needed?  This flag
   * is set only when the whole display is deemed dirty (e.g. when
   * switching back from another application); it is not meant for
   * regular redraws.
   */
  bool dirty = true;

public:
  explicit Display(unsigned antialiasing_samples = 0)
    :EGL::GbmDisplay(GetDriFD()),
     EGL::Display(GetGbmDevice(), antialiasing_samples),
     OpenGL::Display(antialiasing_samples) {}

  void SetDirty() noexcept {
    dirty = true;
  }

  bool CheckDirty() noexcept {
    return std::exchange(dirty, false);
  }
};

#elif defined(USE_WAYLAND)

class Display
  : public Wayland::Display, public EGL::Display, public OpenGL::Display
{
public:
  explicit Display(unsigned antialiasing_samples = 0)
    :EGL::Display(GetWaylandDisplay(), antialiasing_samples),
     OpenGL::Display(antialiasing_samples) {}
};

#elif defined(ENABLE_SDL)

class Display
  : public SDL::Display
#ifdef ENABLE_OPENGL
  , public OpenGL::Display
#endif
{
public:
  explicit Display(unsigned antialiasing_samples = 0)
    :SDL::Display(antialiasing_samples)
#ifdef ENABLE_OPENGL
    , OpenGL::Display(antialiasing_samples)
#endif
  {}
};

#elif defined(USE_GDI)

class Display : public GDI::Display {
public:
  using GDI::Display::Display;
};

#else

class Display {};

#endif // !USE_EGL

/**
 * Drop the DRM master lease and automatically reacquire it at the end
 * of the scope.  This is necessary while a subprocess runs that needs
 * to be DRM master.
 */
class ScopeDropMaster {
#ifdef MESA_KMS
  Display &display;

public:
  explicit ScopeDropMaster(Display &_display) noexcept
    :display(_display)
  {
      display.DropMaster();
  }

  ~ScopeDropMaster() noexcept {
      display.SetMaster();

      /* if we regain the master lease, redraw as soon as possible,
         because the display contents may still be there from the
         previous DRM master */
      display.SetDirty();
  }
#else
public:
  ScopeDropMaster(Display &) noexcept {}
  ~ScopeDropMaster() noexcept = default;
#endif

  ScopeDropMaster(const ScopeDropMaster &) = delete;
  ScopeDropMaster &operator=(const ScopeDropMaster &) = delete;
};

} // namespace UI
