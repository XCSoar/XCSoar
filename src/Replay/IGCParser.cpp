/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "DateTime.hpp"

#include <string.h>
#include <stdio.h>

/**
 * Character table for base-36.
 */
static const char c36[] = "0123456789abcdefghijklmnopqrstuvwxyz";

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
  int DegLat, DegLon;
  int MinLat, MinLon;
  char NoS, EoW;
  int iAltitude, iPressureAltitude;
  int Hour = 0;
  int Minute = 0;
  int Second = 0;
  int lfound =
      sscanf(buffer, "B%02d%02d%02d%02d%05d%c%03d%05d%cA%05d%05d",
      &Hour, &Minute, &Second, &DegLat, &MinLat, &NoS, &DegLon,
      &MinLon, &EoW, &iPressureAltitude, &iAltitude);

  if (lfound == EOF)
    return false;

  if (lfound != 11)
    return false;

  fixed Latitude = fixed(DegLat) + fixed(MinLat) / 60000;
  if (NoS == 'S')
    Latitude *= -1;

  fixed Longitude = fixed(DegLon) + fixed(MinLon) / 60000;
  if (EoW == 'W')
    Longitude *= -1;

  fix.location.Latitude = Angle::degrees(Latitude);
  fix.location.Longitude = Angle::degrees(Longitude);

  fix.gps_altitude = fixed(iAltitude);
  fix.pressure_altitude = fixed(iPressureAltitude);

  // some loggers drop out GPS altitude, so when this happens, revert
  // to pressure altitude
  if ((iPressureAltitude != 0) && (iAltitude==0)) {
    fix.gps_altitude = fix.pressure_altitude;
  }

  fix.time = fixed(Hour * 3600 + Minute * 60 + Second);
  return true;
}
