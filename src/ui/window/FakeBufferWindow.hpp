// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PaintWindow.hpp"

/**
 * Emulation of the #BufferWindow API without actually buffering.
 */
class FakeBufferWindow : public PaintWindow {
protected:
  /**
   * Determines whether this class maintains a persistent buffer which
   * allows incremental drawing in each frame.
   */
  static constexpr bool IsPersistent() {
    return false;
  }

  virtual void OnPaintBuffer(Canvas &canvas) noexcept = 0;

  /* virtual methods from class Window */
  void OnPaint(Canvas &canvas) noexcept override {
    OnPaintBuffer(canvas);
  }
};
