// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"
#include "ui/window/Window.hpp"

class FixedWindowWidget : public WindowWidget {
public:
  using WindowWidget::WindowWidget;

  PixelSize GetMinimumSize() const noexcept override {
    return GetWindow().GetSize();
  }
};
