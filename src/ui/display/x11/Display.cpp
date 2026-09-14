// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "ui/dim/Size.hpp"

/* kludges to work around namespace collisions with X11 headers */
#define Font X11Font
#define Window X11Window
#define Display X11Display
#include <X11/Xlib.h>
#undef Font
#undef Window
#undef Display

#include <stdexcept>

namespace X11 {

Display::Display([[maybe_unused]] unsigned antialiasing_samples)
  :display(XOpenDisplay(nullptr))
{
  if (display == nullptr)
    throw std::runtime_error("XOpenDisplay() failed");
}

Display::~Display() noexcept
{
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
