// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Generator.hpp"
#include "time/BrokenDateTime.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Util.hpp"
#include "util/ASCII.hxx"

#include <cassert>
#include <string.h>
#include <stdio.h>

void
FormatIGCTaskTimestamp(char *buffer, const BrokenDateTime &date_time,
                       unsigned number_of_turnpoints) noexcept
{
  assert(date_time.IsPlausible());

  sprintf(buffer, "C%02u%02u%02u%02u%02u%02u0000000000%02u",
          // DD  MM  YY  HH  MM  SS  DD  MM  YY IIII TT
          date_time.day,
          date_time.month,
          date_time.year % 100,
          date_time.hour,
          date_time.minute,
          date_time.second,
          number_of_turnpoints - 2);
}

char *
FormatIGCLocation(char *buffer, const GeoPoint &location) noexcept
{
  char latitude_suffix = location.latitude.IsNegative() ? 'S' : 'N';
  unsigned latitude =
    (unsigned)uround(fabs(location.latitude.Degrees() * 60000));

  char longitude_suffix = location.longitude.IsNegative() ? 'W' : 'E';
  unsigned longitude =
    (unsigned)uround(fabs(location.longitude.Degrees() * 60000));

  sprintf(buffer, "%02u%05u%c%03u%05u%c",
          latitude / 60000, latitude % 60000, latitude_suffix,
          longitude / 60000, longitude % 60000, longitude_suffix);

  return buffer + strlen(buffer);
}

void
FormatIGCTaskTurnPoint(std::span<char> dest, const GeoPoint &location,
                       const char *name) noexcept
{
  char *p = dest.data();
  char *const end = p + dest.size();
  *p++ = 'C';
  p = FormatIGCLocation(p, location);
  p = CopyASCIIUpper(p, end - p - 1, name);
  *p = '\0';
}
