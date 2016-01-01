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

#include "WaypointReaderWinPilot.hpp"
#include "Units/System.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Util/ExtractParameters.hpp"
#include "Util/StringAPI.hxx"
#include "Util/NumberParser.hpp"
#include "Util/Macros.hpp"

static bool
ParseAngle(const TCHAR* src, Angle& dest, const bool lat)
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
  double sec;
  if (*endptr == ':') {
    /* 00..59 */
    long l = _tcstol(src, &endptr, 10);
    if (endptr == src || l < 0 || l >= 60)
        return false;

    sec = l / 3600.;
  } else if (*endptr == '.') {
    /* 000..999 */
    long l = _tcstol(src, &endptr, 10);
    if (endptr == src + 1 && l >= 0 && l < 100)
      sec = l / (60. * 10);
    else if (endptr == src + 2 && l >= 0 && l < 1000)
      sec = l / (60. * 100);
    else if (endptr == src + 3 && l >= 0 && l < 10000)
      sec = l / (60. * 1000);
    else
      return false;
  } else
    return false;

  auto value = deg + min / 60. + sec;

  TCHAR sign = *endptr;
  if (sign == 'W' || sign == 'w' || sign == 'S' || sign == 's')
    value = -value;

  // Save angle
  dest = Angle::Degrees(value);
  return true;
}

static bool
ParseRunwayDirection(const TCHAR* src, Runway &dest)
{
  // WELT2000 written files contain a 4-digit runway direction specification
  // at the end of the comment, e.g. "123.50 0927"

  const auto *start = StringFindLast(src, _T(' '));
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

  dest.SetDirectionDegrees(a1);
  return true;
}

static bool
ParseAltitude(const TCHAR *src, double &dest)
{
  // Parse string
  TCHAR *endptr;
  double value = _tcstod(src, &endptr);
  if (endptr == src)
    return false;

  dest = value;

  // Convert to system unit if necessary
  TCHAR unit = *endptr;
  if (unit == 'F' || unit == 'f')
    dest = Units::ToSysUnit(dest, Unit::FEET);

  // Save altitude
  return true;
}

static bool
ParseFlags(const TCHAR* src, Waypoint &dest)
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
      dest.type = Waypoint::Type::AIRFIELD;
      break;
    case 'T':
    case 't':
      dest.flags.turn_point = true;
      break;
    case 'L':
    case 'l':
      // Don't downgrade!
      if (dest.type != Waypoint::Type::AIRFIELD)
        dest.type = Waypoint::Type::OUTLANDING;
      break;
    case 'H':
    case 'h':
      dest.flags.home = true;
      break;
    case 'S':
    case 's':
      dest.flags.start_point = true;
      break;
    case 'F':
    case 'f':
      dest.flags.finish_point = true;
      break;
    }
  }

  return true;
}

bool
WaypointReaderWinPilot::ParseLine(const TCHAR *line, Waypoints &waypoints)
{
  TCHAR ctemp[4096];
  const TCHAR *params[20];
  static constexpr unsigned int max_params = ARRAY_SIZE(params);
  size_t n_params;

  // If (end-of-file)
  if (line[0] == '\0')
    // -> return without error condition
    return true;

  // If comment
  if (line[0] == _T('*')) {
    if (first) {
      first = false;
      welt2000_format = (_tcsstr(line, _T("WRITTEN BY WELT2000")) != nullptr);
    }

    // -> return without error condition
    return true;
  }

  if (_tcslen(line) >= ARRAY_SIZE(ctemp))
    /* line too long for buffer */
    return false;

  GeoPoint location;

  // Get fields
  n_params = ExtractParameters(line, ctemp, params, max_params, true);
  if (n_params < 6)
    return false;

  // Latitude (e.g. 51:15.900N)
  if (!ParseAngle(params[1], location.latitude, true))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!ParseAngle(params[2], location.longitude, false))
    return false;
  location.Normalize(); // ensure longitude is within -180:180

  Waypoint new_waypoint = factory.Create(location);

  // Name (e.g. KAMPLI)
  if (*params[5] == _T('\0'))
    return false;
  new_waypoint.name=params[5];

  // Altitude (e.g. 458M)
  /// @todo configurable behaviour
  if (!ParseAltitude(params[3], new_waypoint.elevation) &&
      !factory.FallbackElevation(new_waypoint))
    return false;

  if (n_params > 6) {
    // Description (e.g. 119.750 Airport)
    new_waypoint.comment=params[6];
    if (welt2000_format)
      ParseRunwayDirection(params[6], new_waypoint.runway);
  }

  // Waypoint Flags (e.g. AT)
  ParseFlags(params[4], new_waypoint);

  waypoints.Append(std::move(new_waypoint));
  return true;
}
