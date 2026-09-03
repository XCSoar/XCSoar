// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifndef USE_MEMORY_CANVAS
#error WindowCanvas is only available with the memory canvas
#endif

#include "ui/window/Window.hpp"
#include "ui/canvas/Canvas.hpp"

/**
 * A #Canvas implementation which allows you to draw directly into a
 * #PaintWindow, outside of the PaintWindow::OnPaint().
 */
class WindowCanvas : public Canvas {
public:
  explicit WindowCanvas(Window &window) noexcept {
    buffer.size = window.GetSize();
  }
};
