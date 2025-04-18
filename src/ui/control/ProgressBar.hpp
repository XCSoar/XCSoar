// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"

class ProgressBar : public PaintWindow {
  unsigned min_value = 0, max_value = 0, value = 0, step_size = 1;

public:
  void SetRange(unsigned min_value, unsigned max_value);

  unsigned GetValue() const {
    return value;
  }

  void SetValue(unsigned value);
  void SetStep(unsigned size);
  void Step();

protected:
  void OnPaint(Canvas &canvas) noexcept override;
};
