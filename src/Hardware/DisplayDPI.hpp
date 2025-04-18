// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#if defined(USE_FB) || defined(ANDROID)
#define HAVE_DPI_DETECTION
#endif

struct UnsignedPoint2D;

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

}
