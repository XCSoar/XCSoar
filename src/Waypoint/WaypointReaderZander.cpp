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

#include "WaypointReaderZander.hpp"
#include "Units/Units.hpp"
#include "Waypoint/Waypoints.hpp"

#include <stdio.h>

bool
WaypointReaderZander::ParseLine(const TCHAR* line, const unsigned linenum,
                              Waypoints &way_points)
{
  // If (end-of-file or comment)
  if (line[0] == '\0' || line[0] == 0x1a ||
      _tcsstr(line, _T("**")) == line ||
      _tcsstr(line, _T("*")) == line)
    // -> return without error condition
    return true;

  // Determine the length of the line
  size_t len = _tcslen(line);
  // If less then 34 characters -> something is wrong -> cancel
  if (len < 34)
    return false;

  GeoPoint location;

  // Latitude (Characters 13-20 // DDMMSS(N/S))
  if (!parseAngle(line + 13, location.latitude, true))
    return false;

  // Longitude (Characters 21-29 // DDDMMSS(E/W))
  if (!parseAngle(line + 21, location.longitude, false))
    return false;

  location.Normalize(); // ensure longitude is within -180:180

  Waypoint new_waypoint(location);
  new_waypoint.file_num = file_num;
  new_waypoint.original_id = 0;

  // Name (Characters 0-12)
  if (!parseString(line, new_waypoint.name, 12))
    return false;

  // Altitude (Characters 30-34 // e.g. 1561 (in meters))
  /// @todo configurable behaviour
  if (!parseAltitude(line + 30, new_waypoint.altitude))
    CheckAltitude(new_waypoint);

  // Description (Characters 35-44)
  if (len > 35)
    parseString(line + 35, new_waypoint.comment, 9);

  // Flags (Characters 45-49)
  if (len < 46 || !parseFlags(line + 45, new_waypoint))
    if (len < 36 || !parseFlagsFromDescription(line + 35, new_waypoint))
      new_waypoint.flags.turn_point = true;

  way_points.append(new_waypoint);
  return true;
}


bool
WaypointReaderZander::parseString(const TCHAR* src, tstring& dest, unsigned len)
{
  if (src[0] == 0)
    return true;

  dest.assign(src);

  // Cut the string after the first space, tab or null character
  size_t found = dest.find_first_of(_T("\t\0"));
  if (found != tstring::npos)
    dest = dest.substr(0, found);

  dest = dest.substr(0, len);
  trim_inplace(dest);
  return true;
}

bool
WaypointReaderZander::parseAngle(const TCHAR* src, Angle& dest, const bool lat)
{
  TCHAR *endptr;

  long deg = _tcstol(src, &endptr, 10);
  if (endptr == src || deg < 0)
    return false;

  long sec = deg % 100;
  if (sec >= 60)
    return false;

  long min = ((deg - sec) % 10000) / 100;
  if (min >= 60)
    return false;

  deg = (deg - min - sec) / 10000;

  // Limit angle to +/- 90 degrees for Latitude or +/- 180 degrees for Longitude
  deg = std::min(deg, lat ? 90L : 180L);

  fixed value = fixed(deg) + fixed(min) / 60 + fixed(sec) / 3600;

  TCHAR sign = *endptr;
  if (sign == 'W' || sign == 'w' || sign == 'S' || sign == 's')
    value = -value;

  // Save angle
  dest = Angle::degrees(value);
  return true;
}

bool
WaypointReaderZander::parseAltitude(const TCHAR* src, fixed& dest)
{
  // Parse string
  TCHAR *endptr;
  double val = _tcstod(src, &endptr);
  if (endptr == src)
    return false;

  // Save altitude
  dest = (fixed)val;
  return true;
}

bool
WaypointReaderZander::parseFlags(const TCHAR* src, Waypoint &dest)
{
  // WP = Waypoint
  // HA = Home Field
  // WA = WP + Airfield
  // LF = Landing Field
  // WL = WP + Landing Field
  // RA = Restricted

  // If flags field exists (this function isn't called otherwise)
  // -> turnpoint needs to be declared as one
  dest.flags.turn_point = false;

  if ((src[0] == 'W' || src[0] == 'w') &&
      (src[1] == 'P' || src[1] == 'p')) {
    dest.flags.turn_point = true;
  } else if ((src[0] == 'H' || src[0] == 'h') &&
             (src[1] == 'A' || src[1] == 'a')) {
    dest.type = Waypoint::TYPE_AIRFIELD;
    dest.flags.turn_point = true;
    dest.flags.home = true;
  } else if ((src[0] == 'W' || src[0] == 'w') &&
             (src[1] == 'A' || src[1] == 'a')) {
    dest.type = Waypoint::TYPE_AIRFIELD;
    dest.flags.turn_point = true;
  } else if ((src[0] == 'L' || src[0] == 'l') &&
             (src[1] == 'F' || src[1] == 'f')) {
    dest.type = Waypoint::TYPE_OUTLANDING;
  } else if ((src[0] == 'W' || src[0] == 'w') &&
             (src[1] == 'L' || src[1] == 'l')) {
    dest.type = Waypoint::TYPE_OUTLANDING;
    dest.flags.turn_point = true;
  } else {
    return false;
  }

  return true;
}

bool
WaypointReaderZander::parseFlagsFromDescription(const TCHAR* src,
                                              Waypoint &dest)
{
  // If the description starts with 1 the waypoint is an airport
  // (usually the description of an airport is the frequency)

  if (src[0] == '1') {
    dest.type = Waypoint::TYPE_AIRFIELD;
    dest.flags.turn_point = true;
    return true;
  }

  return false;
}
