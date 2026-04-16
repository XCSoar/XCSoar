// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/DisplayMetrics.hpp"
#include "Math/Point2D.hpp"
#include "ui/dim/Size.hpp"

#if defined(USE_FB) || defined(ANDROID)
#define HAVE_DPI_DETECTION
#endif

namespace UI { class Display; }

namespace Display {
  /**
   * Sets the displays x/y DPI
   * @param x Number of pixels per logical inch along the screen width
   * @param y Number of pixels per logical inch along the screen height
   */
  void SetForcedDPI(unsigned x_dpi, unsigned y_dpi);

#ifdef HAVE_DPI_DETECTION
/**
 * This function gets called by our UI toolkit (the "Screen" library)
 * after it has determined the DPI value of the screen.
 */
void
ProvideDPI(unsigned x_dpi, unsigned y_dpi) noexcept;

/**
 * This function gets called by our UI toolkit (the "Screen" library)
 * after it has determined the physical dimensions of the screen.
 */
void
ProvideSizeMM(unsigned width_pixels, unsigned height_pixels,
             unsigned width_mm, unsigned height_mm) noexcept;
#endif

/**
 * Returns the number of pixels per logical inch along the screen
 * width and height.
 *
 * @param custom_dpi overide system dpi settings, but not command line dpi
 * @return Number of pixels per logical inch along the screen width/height
 */
[[gnu::const]]
UnsignedPoint2D
GetDPI(const UI::Display &display, unsigned custom_dpi=0) noexcept;

/**
 * Physical DPI from pixel dimensions and physical span in millimetres.
 * Returns {0,0} when any dimension is zero.
 */
[[nodiscard]] constexpr UnsignedPoint2D
PhysicalDpiFromSizeMm(PixelSize size, PixelSize size_mm) noexcept
{
  return DisplayMetrics::PhysicalDpiFromSizeMm(size.width, size.height,
                                               size_mm.width, size_mm.height);
}

/**
 * Same as PhysicalDpiFromSizeMm using the display's reported size and
 * size in millimetres.
 */
UnsignedPoint2D
PhysicalDpiFromDisplayMm(const UI::Display &display) noexcept;

/**
 * Returns the content scale factor when the display uses logical scaling
 * (e.g. Xft.dpi on X11 when physical size is unavailable). Layout and
 * canvas use logical size = physical size / scale so that pen/font
 * scaling matches Wayland. Returns 1 when physical size is known or
 * when not applicable.
 */
[[gnu::const]]
unsigned
GetContentScale(const UI::Display &display) noexcept;

}

namespace Hardware {

/**
 * Forwards to Display::GetContentScale().  Use this name in translation
 * units where the X11 `Display` type macro would shadow UI::Display.
 */
[[gnu::const]]
unsigned
GetContentScale(const UI::Display &display) noexcept;

}
