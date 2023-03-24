// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/Window.hpp"
#include "ui/canvas/Canvas.hpp"

class PaintWindow;

/**
 * A #Canvas implementation which allows you to draw directly into a
 * #PaintWindow, outside of the PaintWindow::OnPaint().
 */
class WindowCanvas : public Canvas {
#ifdef USE_MEMORY_CANVAS
public:
  explicit WindowCanvas(Window &window) {
    const auto window_size = window.GetSize();
    buffer.width = window_size.width;
    buffer.height = window_size.height;
  }

#else /* !USE_MEMORY_CANVAS */

protected:
  HWND wnd;

public:
  explicit WindowCanvas(PaintWindow &window);

  ~WindowCanvas() {
    ::ReleaseDC(wnd, dc);
  }
#endif /* !USE_MEMORY_CANVAS */
};
