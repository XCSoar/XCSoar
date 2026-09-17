// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ReachResult.hpp"

#include <cstdint>

enum class WaypointReachability : uint8_t {
  INVALID,
  UNREACHABLE,
  STRAIGHT,
  TERRAIN,
};

[[nodiscard]]
static constexpr bool
IsReachable(WaypointReachability r) noexcept
{
  switch (r) {
  case WaypointReachability::INVALID:
  case WaypointReachability::UNREACHABLE:
    break;

  case WaypointReachability::STRAIGHT:
  case WaypointReachability::TERRAIN:
    return true;
  }

  return false;
}

/**
 * The reachability of a waypoint (arrival heights plus how they were
 * obtained).  Renderers paint this; they do not compute it.
 */
struct WaypointReach {
  ReachResult result{
    .direct = 0,
    .terrain = 0,
    .terrain_valid = ReachResult::Validity::INVALID,
  };

  WaypointReachability reachability = WaypointReachability::INVALID;

  constexpr bool IsReachable() const noexcept {
    return ::IsReachable(reachability);
  }
};
