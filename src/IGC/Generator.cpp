/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Generator.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Util.hpp"
#include "Util/ASCII.hxx"

#include <assert.h>
#include <string.h>
#include <stdio.h>

void
FormatIGCTaskTimestamp(char *buffer, const BrokenDateTime &date_time,
                       unsigned number_of_turnpoints)
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
FormatIGCLocation(char *buffer, const GeoPoint &location)
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
FormatIGCTaskTurnPoint(char *buffer, const GeoPoint &location,
                       const TCHAR *name)
{
  char *p = buffer;
  *p++ = 'C';
  p = FormatIGCLocation(p, location);
  CopyASCIIUpper(p, name);
}
