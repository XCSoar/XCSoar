// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Waypoint.hpp"

#include <cstdint>

/**
 * SeeYou CUP style numbers.
 * The first row for a type is the one the writer emits. Later rows
 * for the same type are read aliases (airfield styles 4 and 5).
 * A home airfield is written as 4. Thermal hotspots have no style.
 */
struct SeeYouStyle
{
  uint8_t style;
  Waypoint::Type type;
};

inline constexpr SeeYouStyle see_you_styles[] = {
  {1, Waypoint::Type::NORMAL},
  {2, Waypoint::Type::AIRFIELD},
  {3, Waypoint::Type::OUTLANDING},
  {4, Waypoint::Type::AIRFIELD},
  {5, Waypoint::Type::AIRFIELD},
  {6, Waypoint::Type::MOUNTAIN_PASS},
  {7, Waypoint::Type::MOUNTAIN_TOP},
  {8, Waypoint::Type::OBSTACLE},
  {9, Waypoint::Type::VOR},
  {10, Waypoint::Type::NDB},
  {11, Waypoint::Type::TOWER},
  {12, Waypoint::Type::DAM},
  {13, Waypoint::Type::TUNNEL},
  {14, Waypoint::Type::BRIDGE},
  {15, Waypoint::Type::POWERPLANT},
  {16, Waypoint::Type::CASTLE},
  {17, Waypoint::Type::INTERSECTION},
  {18, Waypoint::Type::MARKER},
  {19, Waypoint::Type::REPORTING_POINT},
  {20, Waypoint::Type::PGTAKEOFF},
  {21, Waypoint::Type::PGLANDING},
};

constexpr bool
SeeYouStylesCoverTypes() noexcept
{
  bool seen[unsigned(Waypoint::Type::COUNT)]{};
  for (const auto &entry : see_you_styles)
    seen[unsigned(entry.type)] = true;

  for (unsigned i = 0; i < unsigned(Waypoint::Type::COUNT); ++i) {
    if (i == unsigned(Waypoint::Type::THERMAL_HOTSPOT)) continue;
    if (!seen[i]) return false;
  }
  return true;
}

static_assert(SeeYouStylesCoverTypes(),
              "CUP styles must cover every type except thermal hotspot");
