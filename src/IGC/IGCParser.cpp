/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "IGCParser.hpp"
#include "IGCHeader.hpp"
#include "IGCFix.hpp"
#include "DateTime.hpp"

#include <string.h>
#include <stdio.h>

/**
 * Character table for base-36.
 */
static const char c36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/**
 * Convert a 5 digit logger serial to a 3 letter logger id.
 */
static void
ImportDeprecatedLoggerSerial(char id[4], unsigned serial)
{
  id[0] = c36[(serial / 36 / 36) % 36];
  id[1] = c36[(serial / 36) % 36];
  id[2] = c36[serial % 36];
  id[3] = 0;
}

bool
IGCParseHeader(const char *line, IGCHeader &header)
{
  /* sample from CAI302: "ACAM3OV" */
  /* sample from Colibri: "ALXN13103FLIGHT:1" */

  if (line[0] != 'A')
    return false;

  ++line;
  size_t length = strlen(line);
  if (length < 6)
    return false;

  memcpy(header.manufacturer, line, 3);
  header.manufacturer[3] = 0;
  line += 3;

  char *endptr;
  unsigned long serial = strtoul(line, &endptr, 10);
  if (endptr == line + 5) {
    /* deprecated: numeric serial, 5 digits (e.g. from Colibri) */
    ImportDeprecatedLoggerSerial(header.id, serial);
    line = endptr;
  } else {
    memcpy(header.id, line, 3);
    header.id[3] = 0;
    line += 3;
  }

  const char *colon = strchr(line, ':');
  header.flight = colon != NULL
    ? strtoul(colon + 1, NULL, 10)
    : 0;

  return true;
}

bool
IGCParseDate(const char *line, BrokenDate &date)
{
  if (memcmp(line, "HFDTE", 5) != 0)
    return false;

  line += 5;

  char *endptr;
  unsigned long value = strtoul(line, &endptr, 10);
  if (endptr != line + 6)
    return false;

  date.year = 2000 + value % 100; /* Y2100 bug! */
  date.month = (value / 100) % 100;
  date.day = value / 10000;

  return date.Plausible();
}

bool
IGCParseFix(const char *buffer, IGCFix &fix)
{
  BrokenTime time;
  if (!IGCParseFixTime(buffer, time))
    return false;

  unsigned lat_degrees, lat_minutes, lon_degrees, lon_minutes;
  char lat_char, lon_char, valid_char;
  int gps_altitude, pressure_altitude;

  if (sscanf(buffer + 7, "%02u%05u%c%03u%05u%c%c%05d%05d",
             &lat_degrees, &lat_minutes, &lat_char,
             &lon_degrees, &lon_minutes, &lon_char,
             &valid_char, &pressure_altitude, &gps_altitude) != 9)
    return false;

  if (lat_degrees >= 90 || lat_minutes >= 60000 ||
      (lat_char != 'N' && lat_char != 'S'))
    return false;

  if (lon_degrees >= 180 || lon_minutes >= 60000 ||
      (lon_char != 'E' && lon_char != 'W'))
    return false;

  if (valid_char == 'A')
    fix.gps_valid = true;
  else if (valid_char == 'V')
    fix.gps_valid = false;
  else
    return false;

  fix.location.latitude = Angle::Degrees(fixed(lat_degrees) +
                                         fixed(lat_minutes) / 60000);
  if (lat_char == 'S')
    fix.location.latitude.Flip();

  fix.location.longitude = Angle::Degrees(fixed(lon_degrees) +
                                          fixed(lon_minutes) / 60000);
  if (lon_char == 'W')
    fix.location.longitude.Flip();

  fix.gps_altitude = gps_altitude;
  fix.pressure_altitude = pressure_altitude;

  fix.time = time;

  return true;
}

bool
IGCParseFixTime(const char *buffer, BrokenTime &time)
{
  unsigned hour, minute, second;

  if (sscanf(buffer, "B%02u%02u%02u", &hour, &minute, &second) != 3)
    return false;

  if (hour >= 24 || minute >= 60 || second >= 60)
    return false;

  time = BrokenTime(hour, minute, second);
  return true;
}
