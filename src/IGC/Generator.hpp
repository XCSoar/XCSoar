// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

#include <tchar.h>

struct BrokenDateTime;
struct GeoPoint;

/**
 * Generate a task declaration takeoff line according to IGC GNSS
 * specification 3.6.3
 */
static constexpr const char *
IGCMakeTaskTakeoff() noexcept
{
  return "C0000000N00000000ETAKEOFF";
}

static constexpr const char *
IGCMakeTaskLanding() noexcept
{
  return "C0000000N00000000ELANDING";
}

/**
 * IGC GNSS specification 3.6.1
 */
void
FormatIGCTaskTimestamp(char *buffer, const BrokenDateTime &date_time,
                       unsigned number_of_turnpoints) noexcept;

/**
 * @return a pointer to the end of the buffer
 */
char *
FormatIGCLocation(char *buffer, const GeoPoint &location) noexcept;

void
FormatIGCTaskTurnPoint(std::span<char> dest, const GeoPoint &location,
                       const char *name) noexcept;
