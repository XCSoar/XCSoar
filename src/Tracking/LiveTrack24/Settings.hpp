// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

#include <cstdint>

#include <tchar.h>

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

  bool enabled;
  StaticString<64> server;
  StaticString<64> username;
  StaticString<64> password;

  /**
   * Tracking interval in seconds.
   */
  unsigned interval;

  VehicleType vehicleType;
  StaticString<64> vehicle_name;

  void SetDefaults() {
    enabled = false;
    server = "www.livetrack24.com";
    username.clear();
    password.clear();

    interval = 60;

    vehicleType = VehicleType::GLIDER;
  }
};

} /* namespace LiveTrack24 */
