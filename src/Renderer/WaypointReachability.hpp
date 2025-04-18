// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class WaypointReachability : uint8_t {
  INVALID,
  UNREACHABLE,
  STRAIGHT,
  TERRAIN,
};

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
