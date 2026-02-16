// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Id.hpp"
#include "NMEA/Validity.hpp"
#include "util/StaticString.hxx"

#include <type_traits>

/**
 * The FLARM hardware read-out from PFLAC config sentences.
 */
struct FlarmHardware {
  Validity available;

  StaticString<32> device_type;
  StaticString<64> capabilities;
  FlarmId radio_id;

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

  constexpr void Expire([[maybe_unused]] TimeStamp clock) noexcept {
    /* no expiry; this object will be cleared only when the device
       connection is lost */
  }
};

static_assert(std::is_trivial<FlarmHardware>::value, "type is not trivial");
