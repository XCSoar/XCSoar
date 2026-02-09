// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContainerWindow.hpp"
#include "ui/canvas/Color.hpp"

/**
 * A #ContainerWindow with a solid background color, optionally
 * rendered as a vertical gradient when two distinct colors are set.
 */
class SolidContainerWindow : public ContainerWindow {
  Color background_color;
  Color gradient_top_color;
  bool has_gradient = false;

public:
  void Create(ContainerWindow &parent, PixelRect rc, Color _color,
              const WindowStyle style=WindowStyle()) {
    background_color = _color;
    gradient_top_color = _color;
    has_gradient = false;
    ContainerWindow::Create(parent, rc, style);
  }

  void SetBackgroundColor(Color _color) {
    background_color = _color;
    gradient_top_color = _color;
    has_gradient = false;
    Invalidate();
  }

  /**
   * Set a lighter top color to enable a subtle vertical gradient
   * from the given color at the top to background_color at the
   * bottom.  On platforms without OpenGL/EYE_CANDY, background_color
   * is used as a solid fallback.
   */
  void SetGradientTopColor(Color _color) {
    gradient_top_color = _color;
    has_gradient = true;
    Invalidate();
  }

protected:
  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};
