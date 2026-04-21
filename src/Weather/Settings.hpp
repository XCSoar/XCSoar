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
    password = "Duodiscus2026!";
  }
};

struct XCThermSettings {
#ifdef HAVE_HTTP
  bool enabled;
  bool show_on_main_map;
#endif

  WeatherCredentialsSettings credentials;
  unsigned model;
  unsigned parameter;
  unsigned wave_height;
  unsigned vertical_wind_agl;

  void SetDefaults() noexcept {
#ifdef HAVE_HTTP
    enabled = true;
    show_on_main_map = false;
#endif

    credentials.SetDefaults();
    model = 0;
    parameter = 0;
    wave_height = 3000;
    vertical_wind_agl = 100;
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
