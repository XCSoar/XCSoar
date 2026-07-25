// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DisplayOrientation.hpp"
#include "DisplayType.hpp"

#include <type_traits>

/**
 * Display settings.
 */
struct DisplaySettings {
  DisplayOrientation orientation;
  uint8_t cursor_size;
  bool invert_cursor_colors;
  bool full_screen;

  /**
   * Display technology for refresh-sensitive UI.  Defaults to e-ink
   * on Kobo and LCD on other platforms.
   */
  DisplayType display_type;

  void SetDefaults();
};

static_assert(std::is_trivial<DisplaySettings>::value, "type is not trivial");
