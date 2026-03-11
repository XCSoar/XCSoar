// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenDateTime.hpp"
#include "time/BrokenTime.hpp"

#include <cstdint>

/**
 * The state of weather display on the user interface.
 */
struct WeatherUIState {
  enum class EDLStatus : uint8_t {
    DISABLED,
    IDLE,
    LOADING,
    READY,
    ERROR,
  };

  /**
   * The map index being displayed.  -1 means no weather map (normal
   * terrain display).
   */
  int map;

  /**
   * The weather forecast time that shall be displayed.
   */
  BrokenTime time;

  BrokenDateTime forecast_datetime;

  /**
   * Requested altitude in meters MSL.
   */
  int edl_altitude;

  /**
   * Nearest supported EDL pressure level in Pascal.
   */
  unsigned edl_isobar;

  bool edl_enabled;

  EDLStatus edl_status;

  void Clear() {
    map = -1;
    time = BrokenTime::Invalid();
    forecast_datetime = BrokenDateTime::Invalid();
    edl_altitude = 3000;
    edl_isobar = 70000;
    edl_enabled = false;
    edl_status = EDLStatus::DISABLED;
  }
};
