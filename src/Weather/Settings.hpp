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

  /**
   * Keeps the EDL overlay visible on normal map pages.
   */
  bool show_on_main_map;
#endif

  void SetDefaults() {
#ifdef HAVE_HTTP
    enabled = true;
    show_on_main_map = false;
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
