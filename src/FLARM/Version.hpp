// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Validity.hpp"
#include "time/Stamp.hpp"
#include "util/StaticString.hxx"

#include <type_traits>

/**
 * The FLARM version read from the PFLAV sentence.
 */
struct FlarmVersion {
  Validity available;

  StaticString<7> hardware_version, software_version;
  StaticString<19> obstacle_version;

  constexpr void Clear() noexcept {
    available.Clear();
  }

  constexpr void Complement(const FlarmVersion &add) noexcept {
    if (available.Complement(add.available)) {
      hardware_version = add.hardware_version;
      software_version = add.software_version;
      obstacle_version = add.obstacle_version;
    }
  }

  constexpr void Expire([[maybe_unused]] TimeStamp clock) noexcept {
    /* no expiry; this object will be cleared only when the device
       connection is lost */
  }
};

static_assert(std::is_trivial<FlarmVersion>::value, "type is not trivial");
