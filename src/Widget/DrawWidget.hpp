// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

#include <functional>
#include <memory>

#include <tchar.h>

struct PixelRect;
class Canvas;

/**
 * A #Widget that draws using a caller-specified function.
 */
class DrawWidget : public WindowWidget {
  using DrawFunction = std::function<void(Canvas &canvas, const PixelRect &rc)>;
  DrawFunction draw_function;

public:
  DrawWidget(DrawFunction _draw_function) noexcept
    :draw_function(std::move(_draw_function)) {}

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override {
    return false;
  }
};
