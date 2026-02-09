// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"

struct DialogLook;

/**
 * A horizontal black line.
 */
class HLine : public PaintWindow {
  const DialogLook &look;

public:
  HLine(const DialogLook &_look):look(_look) {}

protected:
  void OnPaint(Canvas &canvas) noexcept override;
};
