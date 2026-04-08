// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Generator.hpp"
#include "time/BrokenDateTime.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Util.hpp"
#include "util/ASCII.hxx"
#include "util/StringFormat.hpp"

#include <cassert>
#include <string.h>

void
FormatIGCTaskTimestamp(std::span<char> buffer,
                       const BrokenDateTime &date_time,
                       unsigned number_of_turnpoints) noexcept
{
  assert(date_time.IsPlausible());

  if (buffer.empty() || number_of_turnpoints <= 2) {
    if (!buffer.empty())
      buffer.front() = '\0';
    return;
  }

  const int written = StringFormat(buffer.data(), buffer.size(),
                                   "C%02u%02u%02u%02u%02u%02u0000000000%02u",
                                   // DD  MM  YY  HH  MM  SS  DD  MM  YY IIII TT
                                   date_time.day,
                                   date_time.month,
                                   date_time.year % 100,
                                   date_time.hour,
                                   date_time.minute,
                                   date_time.second,
                                   number_of_turnpoints - 2);
  if (written < 0 || static_cast<size_t>(written) >= buffer.size()) {
    buffer.front() = '\0';
    assert(false);
  }
}

char *
FormatIGCLocation(std::span<char> buffer, const GeoPoint &location) noexcept
{
  if (buffer.empty())
    return nullptr;

  char latitude_suffix = location.latitude.IsNegative() ? 'S' : 'N';
  unsigned latitude =
    (unsigned)uround(fabs(location.latitude.Degrees() * 60000));

  char longitude_suffix = location.longitude.IsNegative() ? 'W' : 'E';
  unsigned longitude =
    (unsigned)uround(fabs(location.longitude.Degrees() * 60000));

  const int written = StringFormat(buffer.data(), buffer.size(),
                                   "%02u%05u%c%03u%05u%c",
                                   latitude / 60000, latitude % 60000, latitude_suffix,
                                   longitude / 60000, longitude % 60000, longitude_suffix);
  if (written < 0 || static_cast<size_t>(written) >= buffer.size())
    return nullptr;

  return buffer.data() + written;
}

void
FormatIGCTaskTurnPoint(std::span<char> dest, const GeoPoint &location,
                       const char *name) noexcept
{
  if (dest.empty())
    return;

  char *p = dest.data();
  char *const end = p + dest.size();
  *p++ = 'C';
  p = FormatIGCLocation({p, static_cast<size_t>(end - p)}, location);
  if (p == nullptr || p >= end) {
    dest.front() = '\0';
    return;
  }
  p = CopyASCIIUpper(p, end - p - 1, name);
  *p = '\0';
}
