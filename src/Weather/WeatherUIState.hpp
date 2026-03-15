// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenDateTime.hpp"
#include "time/BrokenTime.hpp"

#include <chrono>
#include <cstdint>

/**
 * The state of weather display on the user interface.
 */
struct EDLWeatherUIState {
  enum class Status : uint8_t {
    DISABLED,
    IDLE,
    LOADING,
    READY,
    ERROR,
  };

  BrokenDateTime forecast_datetime;

  /**
   * Nearest supported EDL pressure level in Pascal.
   */
  unsigned isobar;

  bool dedicated_page_entered;
  bool dedicated_page_suspended_for_pan;
  bool enabled;

  Status status;

  void Clear() noexcept {
    forecast_datetime = BrokenDateTime::Invalid();
    isobar = 70000;
    dedicated_page_entered = false;
    dedicated_page_suspended_for_pan = false;
    enabled = false;
    status = Status::DISABLED;
  }

  void StepForecast(std::chrono::hours delta) noexcept {
    forecast_datetime = forecast_datetime + delta;
  }

  void SelectIsobar(unsigned new_isobar) noexcept {
    isobar = new_isobar;
  }
};

struct WeatherUIState {
  /**
   * The map index being displayed.  -1 means no weather map (normal
   * terrain display).
   */
  int map;

  /**
   * The weather forecast time that shall be displayed.
   */
  BrokenTime time;

  EDLWeatherUIState edl;

  void Clear() noexcept {
    map = -1;
    time = BrokenTime::Invalid();
    edl.Clear();
  }
};
