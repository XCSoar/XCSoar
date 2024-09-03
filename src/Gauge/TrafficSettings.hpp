// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct TrafficSettings {
  /** Show traffic radar if traffic present? */
  bool enable_gauge;

  /** Automatically close the traffic dialog when no traffic present? */
  bool auto_close_dialog;

  bool auto_zoom;

  bool north_up;

  /** Location of Flarm radar */
  enum class GaugeLocation : uint8_t {
    AUTO,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    CENTER_TOP,
    CENTER_BOTTOM,
    TOP_LEFT_AVOID_IB,
    TOP_RIGHT_AVOID_IB,
    BOTTOM_LEFT_AVOID_IB,
    BOTTOM_RIGHT_AVOID_IB,
    CENTER_TOP_AVOID_IB,
    CENTER_BOTTOM_AVOID_IB,
  } gauge_location;

  void SetDefaults() noexcept;
};
