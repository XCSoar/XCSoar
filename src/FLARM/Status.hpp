// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Traffic.hpp"
#include "NMEA/Validity.hpp"

#include <type_traits>

/**
 * The FLARM operation status read from the PFLAU sentence.
 */
struct FlarmStatus {
  enum class GPSStatus: uint8_t {
    NONE = 0,
    GPS_2D = 1,
    GPS_3D = 2,
  };

  /** Number of received FLARM devices */
  unsigned short rx;
  /** Transmit status */
  bool tx;

  /** GPS status */
  GPSStatus gps;

  /** Alarm level of FLARM (0-3) */
  FlarmTraffic::AlarmType alarm_level;

  /** Is FLARM information available? */
  Validity available;

  constexpr void Clear() noexcept {
    available.Clear();
  }

  constexpr void Complement(const FlarmStatus &add) noexcept {
    if (!available && add.available)
      *this = add;
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    available.Expire(clock, std::chrono::seconds(10));
  }
};

static_assert(std::is_trivial<FlarmStatus>::value, "type is not trivial");
