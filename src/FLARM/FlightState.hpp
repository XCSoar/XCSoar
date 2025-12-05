// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Validity.hpp"
#include <type_traits>

/**
 * Flight and IGC recording state information from PFLAJ sentence.
 */
struct FlarmFlightState {
  enum class FlightState: uint8_t {
    ON_GROUND = 0,
    IN_FLIGHT = 1,
  };

  enum class FlightRecorderState: uint8_t {
    OFF = 0,
    RECORDING = 1,
    BAROMETRIC_ONLY = 2,
  };

  /** Flight state: On ground or in flight */
  FlightState flight_state;

  /** IGC Flight recorder state: OFF, Recording, or Barometric only */
  FlightRecorderState recorder_state;

  /** Is this data valid? */
  Validity available;

  constexpr void Clear() noexcept {
    available.Clear();
    flight_state = FlightState::ON_GROUND;
    recorder_state = FlightRecorderState::OFF;
  }

  constexpr void Complement(const FlarmFlightState &add) noexcept {
    if (available.Complement(add.available)) {
      flight_state = add.flight_state;
      recorder_state = add.recorder_state;
    }
  }

  constexpr void Expire([[maybe_unused]] TimeStamp clock) noexcept {
    /* no expiry; this object will be cleared only when the device
       connection is lost */
  }
};

static_assert(std::is_trivial<FlarmFlightState>::value, "type is not trivial");


