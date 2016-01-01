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

#include "WaypointReaderSeeYou.hpp"
#include "Units/System.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Util/ExtractParameters.hpp"
#include "Util/Macros.hpp"

#include <stdlib.h>

static bool
ParseAngle(const TCHAR* src, Angle& dest, const bool lat)
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

  auto value = deg + min / 60. + l / 60000.;

  TCHAR sign = *endptr;
  if (sign == 'W' || sign == 'w' || sign == 'S' || sign == 's')
    value = -value;

  // Save angle
  dest = Angle::Degrees(value);
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
ParseDistance(const TCHAR *src, double &dest)
{
  // Parse string
  TCHAR *endptr;
  double value = _tcstod(src, &endptr);
  if (endptr == src)
    return false;

  dest = value;

  // Convert to system unit if necessary, assume m as default
  TCHAR* unit = endptr;
  if (StringIsEqualIgnoreCase(unit, _T("ml")))
    dest = Units::ToSysUnit(dest, Unit::STATUTE_MILES);
  else if (StringIsEqualIgnoreCase(unit, _T("nm")))
    dest = Units::ToSysUnit(dest, Unit::NAUTICAL_MILES);
  else if (StringIsEqualIgnoreCase(unit, _T("ft")))
    dest = Units::ToSysUnit(dest, Unit::FEET);

  // Save distance
  return true;
}

static bool
ParseStyle(const TCHAR* src, Waypoint::Type &type)
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
    type = Waypoint::Type::OUTLANDING;
    break;
  case 2:
  case 4:
  case 5:
    type = Waypoint::Type::AIRFIELD;
    break;
  case 6:
    type = Waypoint::Type::MOUNTAIN_PASS;
    break;
  case 7:
    type = Waypoint::Type::MOUNTAIN_TOP;
    break;
  case 8:
    type = Waypoint::Type::OBSTACLE;
    break;
  case 11:
  case 16:
    type = Waypoint::Type::TOWER;
    break;
  case 13:
    type = Waypoint::Type::TUNNEL;
    break;
  case 14:
    type = Waypoint::Type::BRIDGE;
    break;
  case 15:
    type = Waypoint::Type::POWERPLANT;
    break;
  }

  return true;
}

bool
WaypointReaderSeeYou::ParseLine(const TCHAR* line, Waypoints &waypoints)
{
  enum {
    iName = 0,
    iLatitude = 3,
    iLongitude = 4,
    iElevation = 5,
    iStyle = 6,
    iRWDir = 7,
    iRWLen = 8,
    iFrequency = 9,
    iDescription = 10,
  };

  if (first) {
    first = false;

    /* skip first line if it doesn't begin with a quotation character
       (usually the field order line) */
    if (line[0] != _T('\"'))
      return true;
  }

  // If (end-of-file or comment)
  if (StringIsEmpty(line) ||
      StringStartsWith(line, _T("*")))
    // -> return without error condition
    return true;

  TCHAR ctemp[4096];
  if (_tcslen(line) >= ARRAY_SIZE(ctemp))
    /* line too long for buffer */
    return false;

  // If task marker is reached ignore all following lines
  if (StringStartsWith(line, _T("-----Related Tasks-----")))
    ignore_following = true;
  if (ignore_following)
    return true;

  // Get fields
  const TCHAR *params[20];
  size_t n_params = ExtractParameters(line, ctemp, params,
                                      ARRAY_SIZE(params), true, _T('"'));

  // Check if the basic fields are provided
  if (iName >= n_params ||
      iLatitude >= n_params ||
      iLongitude >= n_params)
    return false;

  GeoPoint location;

  // Latitude (e.g. 5115.900N)
  if (!ParseAngle(params[iLatitude], location.latitude, true))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!ParseAngle(params[iLongitude], location.longitude, false))
    return false;

  location.Normalize(); // ensure longitude is within -180:180

  Waypoint new_waypoint = factory.Create(location);

  // Name (e.g. "Some Turnpoint")
  if (*params[iName] == _T('\0'))
    return false;
  new_waypoint.name = params[iName];

  // Elevation (e.g. 458.0m)
  /// @todo configurable behaviour
  if ((iElevation >= n_params ||
      !ParseAltitude(params[iElevation], new_waypoint.elevation)) &&
      !factory.FallbackElevation(new_waypoint))
    return false;

  // Style (e.g. 5)
  if (iStyle < n_params)
    ParseStyle(params[iStyle], new_waypoint.type);

  new_waypoint.flags.turn_point = true;

  // Frequency & runway direction/length (for airports and landables)
  // and description (e.g. "Some Description")
  if (new_waypoint.IsLandable()) {
    if (iFrequency < n_params)
      new_waypoint.radio_frequency = RadioFrequency::Parse(params[iFrequency]);

    // Runway length (e.g. 546.0m)
    double rwlen = -1;
    if (iRWLen < n_params && ParseDistance(params[iRWLen], rwlen) &&
        rwlen > 0)
      new_waypoint.runway.SetLength(uround(rwlen));

    if (iRWDir < n_params && *params[iRWDir]) {
      TCHAR *end;
      int direction =_tcstol(params[iRWDir], &end, 10);
      if (end == params[iRWDir] || direction < 0 || direction > 360 ||
          (direction == 0 && rwlen <= 0))
        direction = -1;
      else if (direction == 360)
        direction = 0;
      if (direction >= 0)
        new_waypoint.runway.SetDirectionDegrees(direction);
    }
  }

  if (iDescription < n_params) {
    /*
     * This convention was introduced by the OpenAIP
     * project (http://www.openaip.net/), since no waypoint type
     * exists for thermal hotspots.
     */
    if (StringStartsWith(params[iDescription], _T("Hotspot")))
      new_waypoint.type = Waypoint::Type::THERMAL_HOTSPOT;

    new_waypoint.comment = params[iDescription];
  }

  waypoints.Append(std::move(new_waypoint));
  return true;
}
