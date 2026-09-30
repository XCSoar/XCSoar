// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Id.hpp"
#include "time/Validity.hpp"
#include "util/StaticString.hxx"

#include <string_view>
#include <type_traits>

namespace FLARM {

/**
 * PowerFLARM Flex and Fusion give out the flight log only through
 * FLARM Hub on the device Wi-Fi.  FTD-014 names the Fusion
 * "PowerFLARM-Fusion"; Flex follows that hyphenated form.
 *
 * Portable, Core and a classic Flarm stay on the binary data port.
 * Matching the whole "PowerFLARM" name would send those there too.
 */
[[nodiscard]] [[gnu::pure]]
constexpr bool
DeviceTypeNeedsWifiDownload(std::string_view device_type) noexcept
{
  return device_type.find("Flex") != std::string_view::npos ||
         device_type.find("Fusion") != std::string_view::npos;
}

} // namespace FLARM

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
    radio_id.Clear();
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
