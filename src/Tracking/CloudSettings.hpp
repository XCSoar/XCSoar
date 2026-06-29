// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/SkyLines/Features.hpp"
#include "util/TriState.hpp"

#ifdef HAVE_SKYLINES_TRACKING

#include <cstdint>

struct CloudSettings {
  /**
   * Is submitting data to the (experimental) XCSoar Cloud enabled?
   * TriState::UNKNOWN means the user has not yet been asked about
   * it.
   */
  TriState enabled;

  /**
   * Receive traffic from the XCSoar Cloud server (including OGN)?
   */
  bool show_traffic;

  bool show_thermals;

  /**
   * Enable cloud communication while on a "roamed" connection?
   */
  bool roaming;

  uint64_t key;

  void SetDefaults() {
    enabled = TriState::UNKNOWN;
    show_traffic = true;
    show_thermals = true;
    roaming = true;
    key = 0;
  }
};

#endif
