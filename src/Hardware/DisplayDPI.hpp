// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

/**
 * Pair each DPI axis with the matching pixel axis.
 *
 * Some Android panels report portrait pixels with landscape physical
 * size (or the reverse), so fonts scale from the long-edge DPI.
 * Unchanged when the pixel and inch orientations already agree, or
 * when the buffer is square.
 */
[[gnu::const]]
UnsignedPoint2D
AlignDpiToPixelAxes(PixelSize size, UnsignedPoint2D dpi) noexcept;

/**
 * Align physical DPI to the pixel axes, then fall back to
 * #density_dpi when either axis is more than 20% away from it.
 * Android's density is what the OEM tuned; xdpi/ydpi are often
 * garbage.  density_dpi 0 skips the fallback.
 */
[[gnu::const]]
UnsignedPoint2D
SanitizeDisplayDpi(PixelSize size, UnsignedPoint2D physical,
                   unsigned density_dpi) noexcept;

/**
 * When #SanitizeDisplayDpi() falls back to density_dpi, Android Skia
 * may still advance glyphs using the aligned OEM x/y DPI.  Map
 * horizontal extent to layout DPI with
 * sqrt(corrected.x/aligned.x * corrected.y/aligned.y).
 */
[[gnu::const]]
float
AndroidTextScaleX(PixelSize size, UnsignedPoint2D physical,
                  unsigned density_dpi,
                  UnsignedPoint2D corrected) noexcept;

/**
 * Vertical em scale for Android Skia when #SanitizeDisplayDpi() had to
 * fall back: corrected.y / aligned.y (font axis vs OEM y DPI).
 */
[[gnu::const]]
float
AndroidTextScaleY(PixelSize size, UnsignedPoint2D physical,
                  unsigned density_dpi,
                  UnsignedPoint2D corrected) noexcept;

/**
 * Letter-spacing (in em) to widen OEM horizontal advances without
 * squashing glyph outlines the way Paint.setTextScaleX() does.
 * Zero when #AndroidTextScaleX() is 1.
 */
[[gnu::const]]
float
AndroidTextLetterSpacing(PixelSize size, UnsignedPoint2D physical,
                         unsigned density_dpi,
                         UnsignedPoint2D corrected) noexcept;

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
 * Pixel size paired with GetSizeMM() for DPI.  Wayland uses the
 * wl_output mode so compositor scale does not halve the density.
 */
[[gnu::const]]
PixelSize
GetSizeForDPI(const UI::Display &display) noexcept;

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
