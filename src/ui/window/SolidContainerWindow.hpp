// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContainerWindow.hpp"
#include "ui/canvas/Color.hpp"

/**
 * A #ContainerWindow with a solid background color.
 */
class SolidContainerWindow : public ContainerWindow {
  Color background_color;

public:
  void Create(ContainerWindow &parent, PixelRect rc, Color _color,
              const WindowStyle style=WindowStyle()) {
    background_color = _color;
    ContainerWindow::Create(parent, rc, style);
  }

  void SetBackgroundColor(Color _color) {
    background_color = _color;
    Invalidate();
  }

protected:
  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};
