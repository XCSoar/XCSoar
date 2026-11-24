// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct NavigatorSettings {
  unsigned navigator_height;
  unsigned navigator_lite_1_line_height;
  unsigned navigator_lite_2_lines_height;
  unsigned navigator_detailed_height;

  bool navigator_swipe;

  /** Navigator Widget type */
  enum class NavigatorWidgetAltitudeType : uint8_t {
    WP_AltReq,    // e_WP_AltReq
    WP_AltDiff,   // e_WP_AltDiff
    WP_AltArrival // e_WP_H
  } navigator_altitude_type;

  void SetDefaults() noexcept;
};
