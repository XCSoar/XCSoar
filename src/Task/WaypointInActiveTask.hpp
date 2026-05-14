// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Ptr.hpp"

/**
 * True if @p wp is the same object as a turnpoint on the current active
 * ordered task (pointer identity).
 */
[[gnu::pure]]
bool
WaypointPtrInActiveOrderedTask(WaypointPtr wp) noexcept;
