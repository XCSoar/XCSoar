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

  if (info.time_available && info.location_available) {
    StringFormat(buffer, buffer_size,
                 "GPRMC,%s,A,%s,%s,%05.1f,%05.1f,%s,%s",
                 time_buffer,
                 lat_buffer,
                 long_buffer,
                 (double)Units::ToUserUnit(info.ground_speed, Unit::KNOTS),
                 (double)info.track.Degrees(),
                 date_buffer,
                 info.variation_available ? var_buffer : ",");
  } else {
    StringFormat(buffer, buffer_size,
                 "%s",
                 "GPRMC,,V,,,,,,,,,");
  }
}

void
FormatGPGGA(char *buffer, size_t buffer_size, const NMEAInfo &info) noexcept
{
  const GeoPoint location = info.location;
  const BrokenDateTime date_time = info.date_time_utc;

  char lat_buffer[20];
  char long_buffer[20];
  char time_buffer[20];

  FormatLatitude(lat_buffer, sizeof(lat_buffer), location.latitude);
  FormatLongitude(long_buffer, sizeof(long_buffer), location.longitude);
  FormatTime(time_buffer, sizeof(time_buffer), date_time);

  if (info.time_available && info.location_available) {
    StringFormat(buffer, buffer_size,
                 "GPGGA,%s,%s,%s,%u,%02u,%.1f,%.3f,M,,,,0000",
                 time_buffer,
                 lat_buffer,
                 long_buffer,
                 (unsigned)info.gps.fix_quality,
                 info.gps.satellites_used_available ? info.gps.satellites_used : 0,
                 info.gps.hdop,
                 info.gps_altitude);
  } else {
    StringFormat(buffer, buffer_size,
                 "%s",
                 "GPGGA,,,,,,,,,,M,,,,");
  }
}

void
FormatGPGSA(char *buffer, size_t buffer_size, const NMEAInfo &info) noexcept
{
  unsigned gps_status;

  if (!info.location_available)
    gps_status = 1;
  else if (!info.gps_altitude_available)
    gps_status = 2;
  else
    gps_status = 3;

  StaticString<256> sat_ids;
  sat_ids.clear();
  for (unsigned i = 0; i < GPSState::MAXSATELLITES; ++i) {
    if (info.gps.satellite_ids[i] > 0 && info.gps.satellite_ids_available) {
      sat_ids.AppendFormat("%02u,", info.gps.satellite_ids[i]);
    } else {
      sat_ids.append(",");
    }
  }

  if (info.gps.satellite_ids_available && info.location_available) {
    StringFormat(buffer, buffer_size,
                 "GPGSA,A,%u,%s%.1f,%.1f,%.1f",
                 gps_status,
                 sat_ids.c_str(),
                 info.gps.pdop,
                 info.gps.hdop,
                 info.gps.vdop);
  } else {
    StringFormat(buffer, buffer_size,
                 "%s",
                 "GPGSA,A,1,,,,,,,,,,,,,,,");
  }
}

void
FormatPGRMZ(char *buffer, size_t buffer_size, const NMEAInfo &info) noexcept
{
  unsigned gps_status;

  if (!info.location_available)
    gps_status = 1;
  else if (!info.gps_altitude_available)
    gps_status = 2;
  else
    gps_status = 3;

  if (info.baro_altitude_available) {
    StringFormat(buffer, buffer_size,
                "PGRMZ,%.0f,m,%u",
                info.baro_altitude,
                gps_status);
  } else {
    StringFormat(buffer, buffer_size,
                 "%s",
                 "PGRMZ,,m,");
  }
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
