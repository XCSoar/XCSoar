// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "ui/dim/Size.hpp"

#include <X11/Xlib.h>

#include <stdexcept>

namespace X11 {

Display::Display()
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
