// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct BrokenDateTime;
struct GeoPoint;

/**
 * Generate a task declaration takeoff line according to IGC GNSS
 * specification 3.6.3
 */
static inline const char *
IGCMakeTaskTakeoff()
{
  return "C0000000N00000000ETAKEOFF";
}

static inline const char *
IGCMakeTaskLanding()
{
  return "C0000000N00000000ELANDING";
}

/**
 * IGC GNSS specification 3.6.1
 */
void
FormatIGCTaskTimestamp(char *buffer, const BrokenDateTime &date_time,
                       unsigned number_of_turnpoints);

/**
 * @return a pointer to the end of the buffer
 */
char *
FormatIGCLocation(char *buffer, const GeoPoint &location);

void
FormatIGCTaskTurnPoint(char *buffer, const GeoPoint &location,
                       const TCHAR *name);
