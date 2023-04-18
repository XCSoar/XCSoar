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

  snprintf(buffer, buffer_size,
           "GPRMC,%02u%02u%02u,%c,%s,%s,%05.1f,%05.1f,%02u%02u%02u,,",
           now.hour, now.minute, now.second,
           info.location.IsValid() ? 'A' : 'V',
           lat_buffer,
           long_buffer,
           (double)Units::ToUserUnit(info.ground_speed, Unit::KNOTS),
           (double)info.track.Degrees(),
           now.day, now.month, now.year % 100);
}

void
FormatLatitude(char *buffer, size_t buffer_size, Angle latitude) noexcept
{
  // Calculate Latitude sign
  char sign = latitude.IsNegative() ? 'S' : 'N';

  double mlat(latitude.AbsoluteDegrees());

  int dd = (int)mlat;
  // Calculate minutes
  double mins = (mlat - dd) * 60.0;

  // Save the string to the buffer
  snprintf(buffer, buffer_size, "%02d%06.3f,%c", dd, mins, sign);
}

void
FormatLongitude(char *buffer, size_t buffer_size, Angle longitude) noexcept
{
  // Calculate Longitude sign
  char sign = longitude.IsNegative() ? 'W' : 'E';

  double mlong(longitude.AbsoluteDegrees());

  int dd = (int)mlong;
  // Calculate minutes
  double mins = (mlong - dd) * 60.0;
  // Save the string to the buffer
  snprintf(buffer, buffer_size, "%02d%06.3f,%c", dd, mins, sign);
}
