// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * The origin where we loaded the waypoint from.
 */
enum class WaypointOrigin : uint8_t {
  /**
   * Temporary waypoints not stored in any file.
   */
  NONE,

  /**
   * User-defined waypoints stored in "user.cup".
   */
  USER,

  PRIMARY,
  WATCHED,

  /**
   * Waypoints stored in the map file.
   */
  MAP,
};
