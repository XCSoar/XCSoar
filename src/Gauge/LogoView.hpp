// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Font.hpp"

struct PixelRect;
class Canvas;

class LogoView {
  Bitmap logo, big_logo, title, big_title;

#ifndef USE_GDI
  Font font;
#endif

public:
  LogoView() noexcept;

  /**
   * Draws the XCSoar logo and the version number into the Canvas, with
   * a white background.
   *
   * @param canvas the Canvas to draw on
   * @param rc the region within the Canvas to draw into
   */
  void draw(Canvas &canvas, const PixelRect &rc) noexcept;
};
