// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

#include <cstdint>

namespace LiveTrack24 {

struct Settings {
  enum class VehicleType : uint8_t {
    GLIDER = 0,
    PARAGLIDER = 1,
    POWERED_AIRCRAFT = 2,
    HOT_AIR_BALLOON = 3,
    HANGGLIDER_FLEX = 4,
    HANGGLIDER_RIGID = 5,
  };

  /** Who may see the track; the values are the protocol's. */
  enum class Privacy : uint8_t {
    PUBLIC = 0,
    PRIVATE = 1,
    FRIENDS = 2,
  };

  bool enabled;
  StaticString<64> username;
  StaticString<64> password;

  /**
   * Tracking interval in seconds.
   */
  unsigned interval;

  VehicleType vehicleType;
  StaticString<64> vehicle_name;

  Privacy privacy;

  void SetDefaults() {
    enabled = false;
    username.clear();
    password.clear();

    interval = 60;

    vehicleType = VehicleType::GLIDER;
    vehicle_name.clear();

    privacy = Privacy::PUBLIC;
  }
};

} /* namespace LiveTrack24 */
