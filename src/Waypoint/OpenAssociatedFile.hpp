// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/RunFile.hpp"

#ifdef HAVE_RUN_FILE

struct Waypoint;

/**
 * Open the waypoint's n-th external file (file=) in a viewer.
 */
void
OpenWaypointAssociatedFile(const Waypoint &waypoint,
                           unsigned index) noexcept;

#endif
