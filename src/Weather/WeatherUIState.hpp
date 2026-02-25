// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/EDL/Levels.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/BrokenTime.hpp"

#include <chrono>
#include <cstdint>

/**
 * Dedicated-page and cursor-bar session flags shared by weather overlays
 * (EDL, RASP, XCTherm).
 */
struct OverlaySession {
  bool page_entered = false;
  bool suspended_for_pan = false;

  /** Cursor-bar selection survives page changes within the app run. */
  bool cursor_initialized = false;

  void Clear() noexcept {
    page_entered = false;
    suspended_for_pan = false;
    cursor_initialized = false;
  }

  /**
   * Mark the dedicated page as entered.
   *
   * @return true if this is the first entry since the last leave
   */
  bool EnterPage() noexcept {
    if (page_entered)
      return false;

    page_entered = true;
    return true;
  }

  void LeavePage() noexcept {
    page_entered = false;
  }

  /**
   * Preserve overlay state while temporarily switching away for pan mode.
   * Only takes effect if a dedicated page is currently entered.
   */
  void SuspendForPan() noexcept {
    suspended_for_pan = page_entered;
  }

  void ResumeAfterPan() noexcept {
    suspended_for_pan = false;
  }

  [[nodiscard]] [[gnu::pure]]
  bool IsSuspendedForPan() const noexcept {
    return suspended_for_pan;
  }

  [[nodiscard]] [[gnu::pure]]
  bool HasPageOwnership() const noexcept {
    return page_entered || suspended_for_pan;
  }
};

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
   * When true, #isobar follows GPS/baro altitude.
   * When false, the user selects the pressure level manually.
   */
  bool level_auto_advance;

  /**
   * Nearest supported EDL pressure level in Pascal.
   */
  unsigned isobar;

  bool enabled;

  OverlaySession session;

  Status status;

  void Clear() noexcept {
    forecast_datetime = BrokenDateTime::Invalid();
    forecast_auto_advance = true;
    level_auto_advance = true;
    isobar = EDL::ISOBARS[0];
    enabled = false;
    session.Clear();
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

  OverlaySession rasp;

  /**
   * Draw contour lines on the RASP weather overlay?
   */
  bool contours = false;

  EDLWeatherUIState edl;

  OverlaySession xctherm;
  struct XCThermCursorState {
    unsigned layer = 0;
    unsigned forecast_utc_hour = 12;
    bool altitude_manual_override = false;
    bool time_manual_override = false;

    void Clear() noexcept {
      layer = 0;
      forecast_utc_hour = 12;
      altitude_manual_override = false;
      time_manual_override = false;
    }
  } xctherm_cursor;

  void Clear() noexcept {
    map = -1;
    time = BrokenTime::Invalid();
    time_auto_advance = true;
    rasp.Clear();
    contours = false;
    edl.Clear();
    xctherm.Clear();
    xctherm_cursor.Clear();
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
