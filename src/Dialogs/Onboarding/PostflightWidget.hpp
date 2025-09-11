// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "Widget/WindowWidget.hpp"

class Canvas;

class PostflightWindow final : public PaintWindow {
protected:
  void OnPaint(Canvas &canvas) noexcept override;
};

class PostflightWidget final : public WindowWidget {
public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
