// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class LabelShape : uint8_t {
  SIMPLE,
  FILLED,
  OUTLINED,
  OUTLINED_INVERTED,
  ROUNDED_WHITE,
  ROUNDED_BLACK,

  /**
   * White background with fully rounded ends, slightly transparent,
   * floating above the map on a soft shadow.
   */
  PILL,
};
