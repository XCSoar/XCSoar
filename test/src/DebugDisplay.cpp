// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Hardware/DisplayDPI.hpp"
#include "ui/dim/Size.hpp"
#include "ui/display/Display.hpp"
#include "Math/Point2D.hpp"
#include "util/PrintException.hxx"

#include <cstdio>

static void
PrintScreenSize([[maybe_unused]] const UI::Display &display) noexcept
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

int main()
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
