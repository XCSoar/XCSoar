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

#include "WaypointReaderWinPilot.hpp"
#include "Units/Units.hpp"
#include "Waypoint/Waypoints.hpp"
#include "IO/TextWriter.hpp"

#include <stdlib.h>

bool
WaypointReaderWinPilot::ParseLine(const TCHAR* line, const unsigned linenum,
                                Waypoints &way_points)
{
  TCHAR ctemp[255];
  const TCHAR *params[20];
  static const unsigned int max_params=sizeof(params)/sizeof(params[0]);
  static bool welt2000_format = false;
  size_t n_params;

  if (linenum == 0)
    welt2000_format = false;

  // If (end-of-file)
  if (line[0] == '\0' || line[0] == 0x1a)
    // -> return without error condition
    return true;

  // If comment
  if (line[0] == _T('*')) {
    if (linenum == 0)
      welt2000_format = (_tcsstr(line, _T("WRITTEN BY WELT2000")) != NULL);
    // -> return without error condition
    return true;
  }

  if (_tcslen(line) >= sizeof(ctemp) / sizeof(ctemp[0]))
    /* line too long for buffer */
    return false;

  GeoPoint location;

  // Get fields
  n_params = ExtractParameters(line, ctemp, params, max_params, true);
  if (n_params < 6)
    return false;

  // Latitude (e.g. 51:15.900N)
  if (!parseAngle(params[1], location.Latitude, true))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!parseAngle(params[2], location.Longitude, false))
    return false;
  location.normalize(); // ensure longitude is within -180:180

  Waypoint new_waypoint(location);
  new_waypoint.FileNum = file_num;
  new_waypoint.original_id = _tcstoul(params[0], NULL, 0);

  // Name (e.g. KAMPLI)
  if (*params[5] == _T('\0'))
    return false;
  new_waypoint.Name=params[5];

  // Altitude (e.g. 458M)
  /// @todo configurable behaviour
  if (!parseAltitude(params[3], new_waypoint.Altitude))
    CheckAltitude(new_waypoint);

  if (n_params > 6) {
    // Description (e.g. 119.750 Airport)
    new_waypoint.Comment=params[6];
    if (welt2000_format)
      parseRunwayDirection(params[6], new_waypoint.RunwayDirection);
  }

  // Waypoint Flags (e.g. AT)
  parseFlags(params[4], new_waypoint);

  way_points.append(new_waypoint);
  return true;
}

bool
WaypointReaderWinPilot::parseAngle(const TCHAR* src, Angle& dest, const bool lat)
{
  // Two format variants:
  // 51:47.841N (DD:MM.mmm // the usual)
  // 51:23:45N (DD:MM:SS)

  TCHAR *endptr;

  long deg = _tcstol(src, &endptr, 10);
  if (endptr == src || *endptr != _T(':') || deg < 0)
    return false;

  // Limit angle to +/- 90 degrees for Latitude or +/- 180 degrees for Longitude
  deg = std::min(deg, lat ? 90L : 180L);

  src = endptr + 1;
  long min = _tcstol(src, &endptr, 10);
  if (endptr == src || min < 0 || min >= 60)
    return false;

  src = endptr + 1;
  fixed sec;
  if (*endptr == ':') {
    /* 00..59 */
    long l = _tcstol(src, &endptr, 10);
    if (endptr == src || l < 0 || l >= 60)
        return false;

    sec = fixed(l) / 3600;
  } else if (*endptr == '.') {
    /* 000..999 */
    long l = _tcstol(src, &endptr, 10);
    if (endptr == src + 1 && l >= 0 && l < 100)
      sec = fixed(l) / (60 * 10);
    else if (endptr == src + 2 && l >= 0 && l < 1000)
      sec = fixed(l) / (60 * 100);
    else if (endptr == src + 3 && l >= 0 && l < 10000)
      sec = fixed(l) / (60 * 1000);
    else
      return false;
  } else
    return false;

  fixed value = fixed(deg) + fixed(min) / 60 + sec;

  TCHAR sign = *endptr;
  if (sign == 'W' || sign == 'w' || sign == 'S' || sign == 's')
    value = -value;

  // Save angle
  dest = Angle::degrees(value);
  return true;
}

bool
WaypointReaderWinPilot::parseRunwayDirection(const TCHAR* src, Angle& dest)
{
  // WELT2000 written files contain a 4-digit runway direction specification
  // at the end of the comment, e.g. "123.50 0927"
  dest = Angle::degrees(fixed_minus_one);

  TCHAR const *start = _tcsrchr(src, _T(' '));
  if (start)
    start++;
  else
    start = src;

  TCHAR *end;
  int value = _tcstol(start, &end, 10);
  if (start == end || *end != _T('\0') || end - start != 4  )
    return false;

  int a1 = (value / 100) * 10;
  int a2 = (value % 100) * 10;
  if (a1 < 0 || a1 > 360 || a2 < 0 || a2 > 360)
    return false;
  if (a1 == 360)
    a1 = 0;
  if (a2 == 360)
    a2 = 0;

  // TODO: WELT2000 generates quite a few entries where
  //       a) a1 == a2 (accept those) and
  //       b) the angles are neither equal or reciprocal (discard those)
  //       Try finding a logic behind this behaviour.
  if (a1 != a2 && (a1 + 180) % 360 != a2)
    return false;

  dest = Angle::degrees(fixed(a1));
  return true;
}

bool
WaypointReaderWinPilot::parseAltitude(const TCHAR* src, fixed& dest)
{
  // Parse string
  TCHAR *endptr;
  double value = _tcstod(src, &endptr);
  if (endptr == src)
    return false;

  dest = fixed(value);

  // Convert to system unit if necessary
  TCHAR unit = *endptr;
  if (unit == 'F' || unit == 'f')
    dest = Units::ToSysUnit(dest, unFeet);

  // Save altitude
  return true;
}

bool
WaypointReaderWinPilot::parseFlags(const TCHAR* src, Waypoint &dest)
{
  // A = Airport
  // T = Turnpoint
  // L = Landable
  // H = Home
  // S = Start point
  // F = Finish point
  // R = Restricted
  // W = Waypoint flag

  for (unsigned i = 0; src[i] != 0; i++) {
    switch (src[i]) {
    case 'A':
    case 'a':
      dest.Type = Waypoint::wtAirfield;
      break;
    case 'T':
    case 't':
      dest.Flags.TurnPoint = true;
      break;
    case 'L':
    case 'l':
      // Don't downgrade!
      if (dest.Type != Waypoint::wtAirfield)
        dest.Type = Waypoint::wtOutlanding;
      break;
    case 'H':
    case 'h':
      dest.Flags.Home = true;
      break;
    case 'S':
    case 's':
      dest.Flags.StartPoint = true;
      break;
    case 'F':
    case 'f':
      dest.Flags.FinishPoint = true;
      break;
    }
  }

  return true;
}
