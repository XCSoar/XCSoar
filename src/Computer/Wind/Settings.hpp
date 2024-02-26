// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/SpeedVector.hpp"
#include "NMEA/Validity.hpp"

#include <type_traits>

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

  void SetDefaults() noexcept;

  constexpr bool IsAutoWindEnabled() const noexcept {
    return circling_wind || zig_zag_wind;
  }

  constexpr bool CirclingWindEnabled() const noexcept {
    return circling_wind;
  }

  constexpr bool ZigZagWindEnabled() const noexcept {
    return zig_zag_wind;
  }

  constexpr unsigned GetLegacyAutoWindMode() const noexcept {
    return (circling_wind ? 0x1 : 0x0) | (zig_zag_wind ? 0x2 : 0x0);
  }

  constexpr void SetLegacyAutoWindMode(unsigned mode) noexcept {
    circling_wind = (mode & 0x1) != 0;
    zig_zag_wind = (mode & 0x2) != 0;
  }
};

static_assert(std::is_trivial<WindSettings>::value, "type is not trivial");
