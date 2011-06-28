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

#include "WaypointReaderSeeYou.hpp"
#include "Units/Units.hpp"
#include "Waypoint/Waypoints.hpp"

#include <stdio.h>

bool
WaypointReaderSeeYou::ParseLine(const TCHAR* line, const unsigned linenum,
                              Waypoints &way_points)
{
  TCHAR ctemp[255];
  const TCHAR *params[20];
  static const unsigned int max_params = sizeof(params) / sizeof(params[0]);
  size_t n_params;

  const unsigned iName = 0;
  const unsigned iLatitude = 3, iLongitude = 4, iElevation = 5;
  const unsigned iStyle = 6, iRWDir = 7, iRWLen = 8;
  const unsigned iFrequency = 9, iDescription = 10;

  static bool ignore_following;
  if (linenum == 0)
    ignore_following = false;

  // If (end-of-file or comment)
  if (line[0] == '\0' || line[0] == 0x1a ||
      _tcsstr(line, _T("**")) == line ||
      _tcsstr(line, _T("*")) == line)
    // -> return without error condition
    return true;

  if (_tcslen(line) >= sizeof(ctemp) / sizeof(ctemp[0]))
    /* line too long for buffer */
    return false;

  // Skip first line if it doesn't begin with a quotation character
  // (usually the field order line)
  if (linenum == 0 && line[0] != _T('\"'))
    return true;

  // If task marker is reached ignore all following lines
  if (_tcsstr(line, _T("-----Related Tasks-----")) == line)
    ignore_following = true;
  if (ignore_following)
    return true;

  // Get fields
  n_params = ExtractParameters(line, ctemp, params, max_params, true, _T('"'));

  // Check if the basic fields are provided
  if (iName >= n_params)
    return false;
  if (iLatitude >= n_params)
    return false;
  if (iLongitude >= n_params)
    return false;

  GeoPoint location;

  // Latitude (e.g. 5115.900N)
  if (!parseAngle(params[iLatitude], location.Latitude, true))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!parseAngle(params[iLongitude], location.Longitude, false))
    return false;

  location.normalize(); // ensure longitude is within -180:180

  Waypoint new_waypoint(location);
  new_waypoint.FileNum = file_num;
  new_waypoint.original_id = 0;

  // Name (e.g. "Some Turnpoint")
  if (*params[iName] == _T('\0'))
    return false;
  new_waypoint.Name = params[iName];

  // Elevation (e.g. 458.0m)
  /// @todo configurable behaviour
  if (iElevation >= n_params ||
      !parseAltitude(params[iElevation], new_waypoint.Altitude))
    CheckAltitude(new_waypoint);

  // Style (e.g. 5)
  /// @todo include peaks with peak symbols etc.
  if (iStyle < n_params)
    parseStyle(params[iStyle], new_waypoint);

  // Frequency & runway direction/length (for airports and landables)
  // and description (e.g. "Some Description")
  if (new_waypoint.IsLandable()) {
    if (iFrequency < n_params)
      new_waypoint.radio_frequency = RadioFrequency::Parse(params[iFrequency]);

    // Runway length (e.g. 546.0m)
    fixed rwlen = fixed_minus_one;
    if (iRWLen < n_params && parseDistance(params[iRWLen], rwlen) &&
        positive(rwlen))
      new_waypoint.runway.SetLength(rwlen);

    if (iRWDir < n_params && *params[iRWDir]) {
      TCHAR *end;
      int direction =_tcstol(params[iRWDir], &end, 10);
      if (end == params[iRWDir] || direction < 0 || direction > 360 ||
          (direction == 0 && !positive(rwlen)))
        direction = -1;
      else if (direction == 360)
        direction = 0;
      if (direction >= 0)
        new_waypoint.runway.SetDirectionDegrees(direction);
    }
  }

  if (iDescription < n_params)
    new_waypoint.Comment = params[iDescription];

  way_points.append(new_waypoint);
  return true;
}

bool
WaypointReaderSeeYou::parseAngle(const TCHAR* src, Angle& dest, const bool lat)
{
  TCHAR *endptr;

  long min = _tcstol(src, &endptr, 10);
  if (endptr == src || *endptr != _T('.') || min < 0)
    return false;

  src = endptr + 1;

  long deg = min / 100;
  min = min % 100;
  if (min >= 60)
    return false;

  // Limit angle to +/- 90 degrees for Latitude or +/- 180 degrees for Longitude
  deg = std::min(deg, lat ? 90L : 180L);

  long l = _tcstol(src, &endptr, 10);
  if (endptr != src + 3 || l < 0 || l >= 1000)
    return false;

  fixed value = fixed(deg) + fixed(min) / 60 + fixed(l) / 60000;

  TCHAR sign = *endptr;
  if (sign == 'W' || sign == 'w' || sign == 'S' || sign == 's')
    value = -value;

  // Save angle
  dest = Angle::degrees(value);
  return true;
}

bool
WaypointReaderSeeYou::parseAltitude(const TCHAR* src, fixed& dest)
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
WaypointReaderSeeYou::parseDistance(const TCHAR* src, fixed& dest)
{
  // Parse string
  TCHAR *endptr;
  double value = _tcstod(src, &endptr);
  if (endptr == src)
    return false;

  dest = fixed(value);

  // Convert to system unit if necessary, assume m as default
  TCHAR* unit = endptr;
  if (_tcsicmp(unit, _T("ml")) == 0)
    dest = Units::ToSysUnit(dest, unStatuteMiles);
  else if (_tcsicmp(unit, _T("nm")) == 0)
    dest = Units::ToSysUnit(dest, unNauticalMiles);

  // Save distance
  return true;
}

bool
WaypointReaderSeeYou::parseStyle(const TCHAR* src, Waypoint &dest)
{
  // 1 - Normal
  // 2 - AirfieldGrass
  // 3 - Outlanding
  // 4 - GliderSite
  // 5 - AirfieldSolid ...

  // Parse string
  TCHAR *endptr;
  long style = _tcstol(src, &endptr, 10);
  if (endptr == src)
    return false;

  // Update flags
  switch (style) {
  case 3:
    dest.Type = Waypoint::wtOutlanding;
    break;
  case 2:
  case 4:
  case 5:
    dest.Type = Waypoint::wtAirfield;
    break;
  case 6:
  case 7:
    dest.Type = Waypoint::wtMountainTop;
    break;
  case 8:
  case 11:
  case 16:
    dest.Type = Waypoint::wtTower;
    break;
  case 13:
    dest.Type = Waypoint::wtTunnel;
    break;
  case 14:
    dest.Type = Waypoint::wtBridge;
    break;
  case 15:
    dest.Type = Waypoint::wtPowerPlant;
    break;
  }

  dest.Flags.TurnPoint = true;

  return true;
}
