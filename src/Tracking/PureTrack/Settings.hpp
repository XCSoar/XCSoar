// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

#include <cstdint>

namespace PureTrack {

struct Settings {
  enum class VehicleType : uint8_t {
    GLIDER,
    PARAGLIDER,
    POWERED_AIRCRAFT,
    HOT_AIR_BALLOON,
    HANGGLIDER_FLEX,
    HANGGLIDER_RIGID,
    COUNT,
  };

  bool enabled;

  /**
   * Insert API endpoint URL.
   */
  StaticString<128> endpoint;

  /**
   * Tracking interval in seconds.
   */
  unsigned interval;

  StaticString<64> app_key;
  StaticString<64> device_id;
  StaticString<64> label;
  StaticString<32> rego;

  VehicleType vehicle_type;

  static constexpr unsigned DEFAULT_INTERVAL = 60;

  void SetDefaults() {
    enabled = false;
    endpoint = "https://puretrack.io/api/insert";
    interval = DEFAULT_INTERVAL;
    app_key.clear();
    device_id.clear();
    label.clear();
    rego.clear();
    vehicle_type = VehicleType::GLIDER;
  }
};

} // namespace PureTrack
