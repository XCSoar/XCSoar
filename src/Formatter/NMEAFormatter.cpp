// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEAFormatter.hpp"
#include "Units/Units.hpp"

void
FormatGPRMC(char *buffer, size_t buffer_size, const NMEAInfo &info) noexcept
{
  char lat_buffer[20];
  char long_buffer[20];

  const GeoPoint location = info.location_available
    ? info.location
    : GeoPoint::Zero();

  FormatLatitude(lat_buffer, sizeof(lat_buffer), location.latitude);
  FormatLongitude(long_buffer, sizeof(long_buffer), location.longitude);

  const BrokenDateTime now = info.time_available &&
    info.date_time_utc.IsDatePlausible()
    ? info.date_time_utc
    : BrokenDateTime::NowUTC();

  StringFormat(buffer, buffer_size,
               "GPRMC,%02u%02u%02u,%c,%s,%s,%05.1f,%05.1f,%02u%02u%02u,,",
               now.hour, now.minute, now.second,
               info.location_available ? 'A' : 'V',
               lat_buffer,
               long_buffer,
               (double)Units::ToUserUnit(info.ground_speed, Unit::KNOTS),
               (double)info.track.Degrees(),
               now.day, now.month, now.year % 100);
}

void
FormatLatitude(char *buffer, size_t buffer_size, Angle latitude) noexcept
{
  snprintf(buffer, buffer_size,
           "%02u%02u.%03u,%c",
           latitude.ToDMM().degrees,
           latitude.ToDMM().minutes,
           latitude.ToDMM().decimal_minutes,
           latitude.IsNegative() ? 'S' : 'N');
}

void
FormatLongitude(char *buffer, size_t buffer_size, Angle longitude) noexcept
{
  snprintf(buffer, buffer_size,
           "%03u%02u.%03u,%c",
           longitude.ToDMM().degrees,
           longitude.ToDMM().minutes,
           longitude.ToDMM().decimal_minutes,
           longitude.IsNegative() ? 'W' : 'E');
}
