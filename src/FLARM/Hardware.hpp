// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Validity.hpp"
#include "util/StaticString.hxx"

#include <type_traits>

/**
 * The FLARM hardware read-out from PFLAC config sentences.
 */
struct FlarmHardware {
  Validity available;

  NarrowString<64> device_type;
  NarrowString<64> capabilities;

  bool isPowerFlarm() noexcept {
    return device_type.Contains("PowerFLARM");
  }

  bool hasADSB() noexcept {
    return capabilities.Contains("XPDR");
  }

  constexpr void Clear() noexcept {
    available.Clear();
  }

  constexpr void Complement(const FlarmHardware &add) noexcept {
    if (!available && add.available)
      *this = add;
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    available.Expire(clock, std::chrono::seconds(10));
  }
};

static_assert(std::is_trivial<FlarmHardware>::value, "type is not trivial");
