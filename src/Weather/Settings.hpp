// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/Features.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_PCMET

#include "PCMet/Settings.hpp"

#endif

struct EDLSettings {
#ifdef HAVE_HTTP
  /**
   * Enables the EDL weather page and downloader.
   */
  bool enabled;
#endif

  void SetDefaults() noexcept {
#ifdef HAVE_HTTP
    enabled = true;
#endif
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

  EDLSettings edl;
#endif

  void SetDefaults() noexcept {
#ifdef HAVE_PCMET
    pcmet.SetDefaults();
#endif

#ifdef HAVE_HTTP
    enable_tim = false;
    edl.SetDefaults();
#endif
  }
};
