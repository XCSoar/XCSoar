// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/EDL/Levels.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/BrokenTime.hpp"

#include <chrono>
#include <cstdint>

/**
 * Density of RASP contour lines.  Each step doubles or halves the
 * number of lines relative to REGULAR (~16 lines across the field
 * range).
 */
enum class ContourDensity : unsigned {
  OFF = 0,
  WIDE = 1,      ///< ~8 lines (spacing × 2)
  REGULAR = 2,   ///< ~16 lines (baseline)
  FINE = 3,      ///< ~32 lines (spacing / 2)
  SUPERFINE = 4, ///< ~64 lines (spacing / 4)
  COUNT
};

/**
 * Convert ContourDensity + a RASP height_scale into a contour_spacing
 * value suitable for RasterRenderer::GenerateImage().  Returns 0 when
 * contours are disabled.
 */
[[gnu::const]] inline unsigned
ContourSpacing(ContourDensity density, unsigned height_scale) noexcept
{
  switch (density) {
  case ContourDensity::OFF:       return 0;
  case ContourDensity::WIDE:      return 1u << (height_scale + 5);
  case ContourDensity::REGULAR:   return 1u << (height_scale + 4);
  case ContourDensity::FINE:      return 1u << (height_scale + 3);
  case ContourDensity::SUPERFINE: return 1u << (height_scale + 2);
  case ContourDensity::COUNT:
    assert(false);
  }
  return 0;
}

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

  /**
   * When true, the RASP dedicated-page overlay is preserved across a
   * temporary pan (full-screen map) instead of being cleared.  Mirrors
   * EDLWeatherUIState::dedicated_page_suspended_for_pan.
   */
  bool rasp_page_suspended_for_pan;

  /**
   * Density of contour lines drawn on the RASP weather overlay.
   */
  ContourDensity contour_density = ContourDensity::OFF;

  EDLWeatherUIState edl;

  void Clear() noexcept {
    map = -1;
    time = BrokenTime::Invalid();
    time_auto_advance = true;
    rasp_page_entered = false;
    rasp_page_suspended_for_pan = false;
    contour_density = ContourDensity::OFF;
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

  /**
   * Suspend the RASP dedicated page while panning, so the overlay is
   * kept instead of being torn down.  Only takes effect if a RASP page
   * is currently entered.
   */
  void SuspendRaspForPan() noexcept {
    rasp_page_suspended_for_pan = rasp_page_entered;
  }

  void ResumeRaspAfterPan() noexcept {
    rasp_page_suspended_for_pan = false;
  }

  [[gnu::pure]]
  bool IsRaspSuspendedForPan() const noexcept {
    return rasp_page_suspended_for_pan;
  }
};
