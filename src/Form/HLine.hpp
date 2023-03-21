// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"

struct DialogLook;

/**
 * A horizontal black line.
 */
class HLine : public PaintWindow {
#ifndef ENABLE_OPENGL
  const DialogLook &look;
#endif

public:
#ifdef ENABLE_OPENGL
  HLine([[maybe_unused]] const DialogLook &_look) {}
#else
  HLine([[maybe_unused]] const DialogLook &_look):look(_look) {}
#endif

protected:
  void OnPaint(Canvas &canvas) noexcept override;
};
