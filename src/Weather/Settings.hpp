// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/Features.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_PCMET

#include "PCMet/Settings.hpp"

#endif

#ifdef HAVE_SKYSIGHT
#include "Skysight/Settings.hpp"
#endif

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

#ifdef HAVE_SKYSIGHT
  SkysightSettings skysight;
#endif

  void SetDefaults() {
#ifdef HAVE_PCMET
    pcmet.SetDefaults();
#endif

#ifdef HAVE_HTTP
    enable_tim = false;
#endif
  }
};
