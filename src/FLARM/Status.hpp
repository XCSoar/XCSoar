// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Traffic.hpp"
#include "FLARM/Id.hpp"
#include "NMEA/Validity.hpp"

#include <type_traits>
#include <cstdint>

/**
 * The FLARM operation status read from the PFLAU sentence.
 * @see FTD-012 Data Port ICD, PFLAU sentence
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

  /**
   * Relative bearing to the most threatening intruder (field 6).
   * Only valid when alarm_level > NONE.
   */
  int16_t relative_bearing;

  /** Alarm type of the most threatening intruder (field 7) */
  uint8_t alarm_type;

  /** Relative vertical distance to threat in metres (field 8) */
  int32_t relative_vertical;

  /** Horizontal distance to threat in metres (field 9) */
  uint32_t relative_distance;

  /** FLARM ID of the most threatening intruder (field 10) */
  FlarmId target_id;

  /** Were the extended fields (6-10) present in the sentence? */
  bool has_extended;

  /** Is FLARM information available? */
  Validity available;

  constexpr void Clear() noexcept {
    available.Clear();
    has_extended = false;
    target_id.Clear();
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
