// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "../../shared/RotatePointer.hpp"

#include <cassert>

namespace UI {

struct Event;

/**
 * This class keeps track of the current mouse position and generates
 * movement events for the #EventQueue.  It is responsible for merging
 * the input of multiple (hot-plugged) mouse devices.
 */
class MergeMouse final {
  RotatePointer rotate;

  unsigned x = 0, y = 0;

  /**
   * Relative mouse wheel movement since last Generate() call.
   */
  int wheel = 0;

  bool down = false;

  bool moved = false, pressed = false, released = false;

  /**
   * The number of pointer input devices.
   */
  unsigned n_pointers = 0;

public:
  MergeMouse() = default;
  MergeMouse(const MergeMouse &) = delete;

  ~MergeMouse() noexcept {
    assert(n_pointers == 0);
  }

  void SetScreenSize(PixelSize screen_size) noexcept;

  void SetDisplayOrientation(DisplayOrientation orientation) {
    rotate.SetDisplayOrientation(orientation);
  }

  void AddPointer() noexcept {
    ++n_pointers;
  }

  void RemovePointer() noexcept {
    assert(n_pointers > 0);

    --n_pointers;
  }

  bool HasPointer() const noexcept {
    return n_pointers > 0;
  }

  void SetDown(bool new_down) noexcept;
  void MoveAbsolute(PixelPoint p) noexcept;
  void MoveAbsolute(int new_x, int new_y,
                    int min_x, int max_x, int min_y, int max_y) noexcept;
  void MoveRelative(PixelPoint d) noexcept;

  void MoveWheel(int d) noexcept {
    wheel += d;
  }

  PixelPoint GetPosition() const noexcept {
    return PixelPoint(x, y);
  }

  Event Generate() noexcept;
};

} // namespace UI
