// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Waypoint.hpp"

/**
 * Untranslated English label for a waypoint type.
 * Indexed with Waypoint::Type; gettext happens in the caller
 * that displays it (AddChoices, or GetWaypointTypeName()).
 */
const char *WaypointTypeLabel(Waypoint::Type type) noexcept;

const char *GetWaypointTypeName(Waypoint::Type type) noexcept;
