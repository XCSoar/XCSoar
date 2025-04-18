// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class DisplayOrientation : uint8_t;

namespace Display {
  void RotateInitialize();

  [[gnu::const]]
  bool RotateSupported();

  /**
   * Change the orientation of the screen.
   */
  bool
  Rotate(DisplayOrientation orientation);

  /**
   * Restores the display rotation setting.
   */
  bool
  RotateRestore();
}
