// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DisplayOrientation.hpp"

#include <type_traits>

/**
 * Display settings.
 */
struct DisplaySettings {
  DisplayOrientation orientation;
  uint8_t cursor_size;
  bool invert_cursor_colors;
  bool full_screen;

  void SetDefaults();
};

static_assert(std::is_trivial<DisplaySettings>::value, "type is not trivial");
