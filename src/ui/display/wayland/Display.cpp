// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

namespace Wayland {

Display::Display()
  :display(wl_display_connect(nullptr))
{
  if (display == nullptr)
    throw std::runtime_error("wl_display_connect() failed");
}

Display::~Display() noexcept
{
  wl_display_disconnect(display);
}

} // namespace Wayland
