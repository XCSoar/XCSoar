// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Font.hpp"

struct PixelRect;
class Canvas;

class LogoView {
  Bitmap logo, big_logo, huge_logo, title, big_title, huge_title;
  Bitmap logo_rgba, big_logo_rgba, huge_logo_rgba;
  Bitmap white_title;

#ifndef USE_GDI
  Font font;
#ifndef NDEBUG
  Font bold_font;
#endif
#endif

public:
  LogoView() noexcept;

  /**
   * Draws the XCSoar logo and the version number into the Canvas.
   *
   * @param canvas the Canvas to draw on
   * @param rc the region within the Canvas to draw into
   * @param dark_mode if true, use light-on-dark colors and white title
   */
  void draw(Canvas &canvas, const PixelRect &rc,
            bool dark_mode = false) noexcept;
};
