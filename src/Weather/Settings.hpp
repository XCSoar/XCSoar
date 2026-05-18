// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/Features.hpp"
#include "net/http/Features.hpp"
#include "util/StaticString.hxx"

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
    email = "xcthermdev@gmail.com";
    password = "DiePerfekteWelle!26";
  }
};

struct XCThermSettings {
#ifdef HAVE_HTTP
  bool enabled;
  bool show_on_main_map;

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
    enabled = true;
    show_on_main_map = false;
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

  XCThermSettings xctherm;

  void SetDefaults() noexcept {
#ifdef HAVE_PCMET
    pcmet.SetDefaults();
#endif

#ifdef HAVE_HTTP
    enable_tim = false;
#endif

    xctherm.SetDefaults();
  }
};
