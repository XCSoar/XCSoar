// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Canvas.hpp"

class Window;

/**
 * A #Canvas implementation to use during WM_PAINT.  Use this class
 * instead of #PaintWindow.get_canvas().
 */
class PaintCanvas : public Canvas {
private:
  Window &window;
  PAINTSTRUCT ps;

public:
  PaintCanvas(Window &_window);
  ~PaintCanvas();

  const PixelRect &get_dirty() const {
    return reinterpret_cast<const PixelRect &>(ps.rcPaint);
  }
};
