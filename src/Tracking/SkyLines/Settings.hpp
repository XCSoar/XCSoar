// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "CloudSettings.hpp"
#include "Features.hpp"
#include "util/TriState.hpp"

#ifdef HAVE_SKYLINES_TRACKING

#include <cstdint>

namespace SkyLinesTracking {

struct Settings {
  bool enabled;

  /**
   * Enable tracking while on a "roamed" connection?
   */
  bool roaming;

  /**
   * Periodically request friend traffic information?
   */
  bool traffic_enabled;

  /**
   * Periodically request near traffic information?
   */
  bool near_traffic_enabled;

  /**
   * Tracking interval in seconds.
   */
  unsigned interval;

  uint64_t key;

  CloudSettings cloud;

  void SetDefaults() {
    enabled = false;
    roaming = true;
    traffic_enabled = false;
    near_traffic_enabled = false;
    interval = 5;
    key = 0;
    cloud.SetDefaults();
  }
};

} /* namespace SkyLinesTracking */

#endif
