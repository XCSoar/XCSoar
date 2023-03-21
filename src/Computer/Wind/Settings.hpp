// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/SpeedVector.hpp"
#include "NMEA/Validity.hpp"

#include <type_traits>

// control of calculations, these only changed by user interface
// but are used read-only by calculations

/** AutoWindMode (not in use) */
enum AutoWindModeBits
{
  /** 0: Manual */
  AUTOWIND_NONE = 0,
  /** 1: Circling */
  AUTOWIND_CIRCLING,
  /** 2: ZigZag */
  AUTOWIND_ZIGZAG,
  /** 3: Both */
};

/**
 * Wind calculator settings
 */
struct WindSettings {
  /**
   * Use the circling algorithm to calculate the wind?
   */
  bool circling_wind;

  /**
   * Use the EKF algorithm to calculate the wind? (formerly known as
   * "zig zag")
   */
  bool zig_zag_wind;

  bool external_wind;

  /**
   * This is the manual wind set by the pilot. Validity is set when
   * changeing manual wind but does not expire.
   */
  SpeedVector manual_wind;
  Validity manual_wind_available;

  void SetDefaults();

  bool IsAutoWindEnabled() const {
    return circling_wind || zig_zag_wind;
  }

  bool CirclingWindEnabled() const {
    return circling_wind;
  }

  bool ZigZagWindEnabled() const {
    return zig_zag_wind;
  }

  unsigned GetLegacyAutoWindMode() const {
    return (circling_wind ? 0x1 : 0x0) | (zig_zag_wind ? 0x2 : 0x0);
  }

  void SetLegacyAutoWindMode(unsigned mode) {
    circling_wind = (mode & 0x1) != 0;
    zig_zag_wind = (mode & 0x2) != 0;
  }
};

static_assert(std::is_trivial<WindSettings>::value, "type is not trivial");
