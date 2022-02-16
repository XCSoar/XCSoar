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
  using EGL::Display::Display;
};

#elif defined(USE_EGL) && defined(USE_X11)

class Display
  : public X11::Display, public EGL::Display,
    public OpenGL::Display {
public:
  Display()
    :EGL::Display(X11::Display::GetXDisplay()) {}
};

#elif defined(USE_GLX) && defined(USE_X11)

class Display : public X11::Display, public OpenGL::Display {
public:
  using X11::Display::Display;
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
  Display()
    :EGL::GbmDisplay(GetDriFD()),
     EGL::Display(GetGbmDevice()) {}

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
  Display()
    :EGL::Display(GetWaylandDisplay()) {}
};

#elif defined(ENABLE_SDL)

class Display
  : public SDL::Display
#ifdef ENABLE_OPENGL
  , public OpenGL::Display
#endif
{
public:
  using SDL::Display::Display;
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
