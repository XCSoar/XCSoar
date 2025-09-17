// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "Widget/WindowWidget.hpp"

class Canvas;

class WelcomeWindow final : public PaintWindow {
private:
  PixelRect xcsoar_link_rect;
  PixelRect github_link_rect;
protected:
  void OnPaint(Canvas &canvas) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
};

class WelcomeWidget final : public WindowWidget {
public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
