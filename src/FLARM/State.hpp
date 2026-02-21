// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Validity.hpp"

#include <type_traits>
#include <cstdint>

/**
 * Flight and IGC recording state from the PFLAJ sentence.
 * @see FTD-012 Data Port ICD, PFLAJ sentence (protocol v8+)
 */
struct FlarmState {
  enum class Flight : uint8_t {
    ON_GROUND = 0,
    IN_FLIGHT = 1,
  };

  enum class Recorder : uint8_t {
    OFF = 0,
    RECORDING = 1,
    BARO_ONLY = 2,
  };

  Validity available;

  Flight flight;
  Recorder recorder;

  constexpr void Clear() noexcept {
    available.Clear();
  }

  constexpr void Complement(const FlarmState &add) noexcept {
    if (available.Complement(add.available)) {
      flight = add.flight;
      recorder = add.recorder;
    }
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    available.Expire(clock, std::chrono::minutes{5});
  }
};

static_assert(std::is_trivial<FlarmState>::value, "type is not trivial");
