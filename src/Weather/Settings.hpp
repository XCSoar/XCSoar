// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/Features.hpp"
#include "net/http/Features.hpp"
#include "util/StaticString.hxx"

#include <cstdint>

#ifdef HAVE_PCMET

#include "PCMet/Settings.hpp"

#endif

struct WeatherCredentialsSettings {
  StaticString<64> email;
  StaticString<64> password;

  bool IsDefined() const noexcept {
    return !email.empty() && !password.empty();
  }

  void SetDefaults() noexcept {
    /* Never embed real credentials in source — they would ship in the
       binary and end up reused across users. Users supply their own
       credentials via Config → Weather → XCTherm. */
    email.clear();
    password.clear();
  }
};

struct RaspSettings {
  /**
   * When true, XCSoar may download/update the configured RASP file in
   * the background when it is missing or out of date (e.g. on entering
   * a RASP map page, or when auto time has no data). Manual Update in
   * Info → Weather → RASP always remains available.
   */
  bool auto_update;

  void SetDefaults() noexcept {
    auto_update = true;
  }
};

struct EdlSettings {
  /**
   * When true, XCSoar may download missing EDL overlay tiles in the
   * background (e.g. on entering an EDL map page, or when changing
   * time/level). Manual Precache day in Info → Weather → EDL remains
   * available when Auto update is off.
   */
  bool auto_update;

  void SetDefaults() noexcept {
    auto_update = true;
  }
};

struct XCThermSettings {
#ifdef HAVE_HTTP
  /**
   * Automatically switch altitude layer and forecast time
   * based on current GPS altitude and UTC time.
   */
  bool auto_switch;
#endif

  WeatherCredentialsSettings credentials;
  unsigned model;
  unsigned parameter;
  unsigned wave_height;
  unsigned vertical_wind_agl;

  /**
   * How many hours of hourly forecasts the Download button fetches,
   * starting from the next full hour. E.g. 12 → download +1h, +2h, ..., +12h.
   *
   * Session-only: always resets to the default on startup. The user can
   * change it via the "Span" button during a session, but the value is
   * never persisted to the profile — by design, so each session starts
   * with the cheap quick-look default.
   */
  unsigned download_span_hours;

  void SetDefaults() noexcept {
#ifdef HAVE_HTTP
    auto_switch = true;
#endif

    credentials.SetDefaults();
    model = 0;
    parameter = 0;
    wave_height = 3000;
    vertical_wind_agl = 100;
    download_span_hours = 1;
  }
};

struct WeatherSettings {
#ifdef HAVE_PCMET
  PCMetSettings pcmet;
#endif

#ifdef HAVE_HTTP
  /**
   * Enable Thermal Information Map?
   */
  bool enable_tim;
#endif

  RaspSettings rasp;
  EdlSettings edl;
  XCThermSettings xctherm;

  void SetDefaults() noexcept {
#ifdef HAVE_PCMET
    pcmet.SetDefaults();
#endif

#ifdef HAVE_HTTP
    enable_tim = false;
#endif

    rasp.SetDefaults();
    edl.SetDefaults();
    xctherm.SetDefaults();
  }
};
