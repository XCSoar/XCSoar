// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEAFormatter.hpp"
#include "Units/Units.hpp"

void
FormatGPRMC(char *buffer, size_t buffer_size, const NMEAInfo &info) noexcept
{
  char lat_buffer[20];
  char long_buffer[20];
  char var_buffer[20];
  char time_buffer[20];
  char date_buffer[20];

  const GeoPoint location = info.location;
  const BrokenDateTime date_time = info.date_time_utc;

  FormatLatitude(lat_buffer, sizeof(lat_buffer), location.latitude);
  FormatLongitude(long_buffer, sizeof(long_buffer), location.longitude);
  FormatVariation(var_buffer, sizeof(var_buffer), info.variation);
  FormatTime(time_buffer, sizeof(time_buffer), date_time);
  FormatDate(date_buffer, sizeof(date_buffer), date_time);

  StringFormat(buffer, buffer_size,
               "GPRMC,%s,%c,%s,%s,%05.1f,%05.1f,%s,%s",
               time_buffer,
               info.location_available ? 'A' : 'V',
               lat_buffer,
               long_buffer,
               (double)Units::ToUserUnit(info.ground_speed, Unit::KNOTS),
               (double)info.track.Degrees(),
               date_buffer,
               info.variation_available ? var_buffer : ",");

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

void
FormatVariation(char *buffer, size_t buffer_size, Angle variation) noexcept
{
  snprintf(buffer, buffer_size,
           "%05.1f,%c",
           variation.AbsoluteDegrees(),
           variation.IsNegative() ? 'W' : 'E');
}

void
FormatTime(char *buffer, size_t buffer_size, BrokenDateTime time) noexcept
{
  snprintf(buffer, buffer_size,
           "%02u%02u%02u.00",
           time.hour, time.minute, time.second);
}

void
FormatDate(char *buffer, size_t buffer_size, BrokenDateTime time) noexcept
{
  snprintf(buffer, buffer_size,
           "%02u%02u%02u",
           time.day, time.month, time.year % 100);
}
