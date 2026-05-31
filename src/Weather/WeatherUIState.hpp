// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/EDL/Levels.hpp"
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
    FAILED,
  };

  BrokenDateTime forecast_datetime;

  /**
   * When true, #forecast_datetime follows GPS time (next UTC hour).
   * When false, the user selects the forecast manually.
   */
  bool forecast_auto_advance;

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
    forecast_auto_advance = true;
    isobar = EDL::ISOBARS[0];
    dedicated_page_entered = false;
    dedicated_page_suspended_for_pan = false;
    enabled = false;
    status = Status::DISABLED;
  }

  void StepForecast(std::chrono::hours delta) noexcept {
    forecast_datetime = forecast_datetime + delta;
  }

  void SelectIsobar(unsigned new_isobar) noexcept {
    if (!EDL::IsSupportedIsobar(new_isobar))
      return;

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

  /**
   * When true, #time follows GPS local time ("Now").
   * When false, the user selects the forecast manually.
   */
  bool time_auto_advance;

  bool rasp_page_entered;

  EDLWeatherUIState edl;

  void Clear() noexcept {
    map = -1;
    time = BrokenTime::Invalid();
    time_auto_advance = true;
    rasp_page_entered = false;
    edl.Clear();
  }

  /**
   * Mark the current RASP page as entered.
   *
   * @return true if this is the first entry since the last leave
   */
  bool EnterRaspDedicatedPage() noexcept {
    if (rasp_page_entered)
      return false;

    rasp_page_entered = true;
    return true;
  }

  /**
   * Resync RASP time when entering a page after leaving it.
   * Keeps manual selection when auto advance is disabled.
   */
  void ResetRaspForDedicatedPage() noexcept {
    if (time_auto_advance)
      time = BrokenTime::Invalid();
  }
};
