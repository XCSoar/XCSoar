// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

struct Waypoint;

/**
 * Profile path of the CUP/CUPX file that loaded @p waypoint, or empty.
 */
[[nodiscard]]
AllocatedPath
GetWaypointSourcePath(const Waypoint &waypoint) noexcept;
