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

#include "Hardware/DisplayDPI.hpp"
#include "ui/dim/Size.hpp"
#include "ui/display/Display.hpp"
#include "Math/Point2D.hpp"
#include "util/PrintException.hxx"

#include <cstdio>

static void
PrintScreenSize(const UI::Display &display) noexcept
{
#if defined(USE_X11) || defined(MESA_KMS) || defined(USE_GDI)
  const auto size = display.GetSize();
  printf("Width: %u px | Height: %u px\n", size.width, size.height);
#endif

#if defined(USE_X11) || defined(MESA_KMS)
  const auto size_mm = display.GetSizeMM();
  printf("Width: %u mm | Height: %u mm\n", size_mm.width, size_mm.height);
#endif
}

static void
PrintDPI(const UI::Display &display) noexcept
{
  const auto dpi = Display::GetDPI(display);
  printf("DPI X: %u | DPI Y: %u\n", dpi.x, dpi.y);
}

int
main(int argc, char **argv)
try {
  const UI::Display display;

  printf("Display Information\n\n");

  PrintScreenSize(display);
  PrintDPI(display);

  return 0;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
