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

#include "WaypointReaderOzi.hpp"
#include "Waypoint/Waypoints.hpp"
#include "IO/LineReader.hpp"
#include "Units/System.hpp"
#include "Util/Macros.hpp"
#include "Util/ExtractParameters.hpp"

#include <stdlib.h>

static bool
ParseAngle(const TCHAR *src, Angle &angle)
{
  TCHAR *endptr;
  double deg = _tcstod(src, &endptr);
  if (endptr == src)
    return false;

  angle = Angle::Degrees(deg);
  return true;
}

static bool
ParseNumber(const TCHAR *src, long &dest)
{
  TCHAR *endptr;
  long temp = _tcstol(src, &endptr, 10);
  if (endptr == src)
    return false;

  dest = temp;
  return true;
}

static bool
ParseString(const TCHAR *src, tstring &dest)
{
  if (src[0] == 0)
    return true;

  dest.assign(src);
  trim_inplace(dest);

  return true;
}

bool
WaypointReaderOzi::ParseLine(const TCHAR *line, Waypoints &way_points)
{
  if (line[0] == '\0')
    return true;

  // Ignore first four header lines
  if (ignore_lines > 0) {
    --ignore_lines;
    return true;
  }

  TCHAR ctemp[255];
  const TCHAR *params[20];
  static constexpr unsigned int max_params = ARRAY_SIZE(params);
  size_t n_params;

  if (_tcslen(line) >= ARRAY_SIZE(ctemp))
    /* line too long for buffer */
    return false;

  // Get fields
  n_params = ExtractParameters(line, ctemp, params, max_params, true, _T('"'));

  // Check if the basic fields are provided
  if (n_params < 15)
    return false;

  GeoPoint location;
  // Latitude (e.g. 5115.900N)
  if (!ParseAngle(params[2], location.latitude))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!ParseAngle(params[3], location.longitude))
    return false;

  location.Normalize(); // ensure longitude is within -180:180

  Waypoint new_waypoint = factory.Create(location);

  long value;
  new_waypoint.original_id = (ParseNumber(params[0], value) ? value : 0);

  if (!ParseString(params[1], new_waypoint.name))
    return false;

  if (ParseNumber(params[14], value) && value != -777)
    new_waypoint.elevation = Units::ToSysUnit(value, Unit::FEET);
  else if (!factory.FallbackElevation(new_waypoint))
    return false;

  // Description
  ParseString(params[10], new_waypoint.comment);

  way_points.Append(std::move(new_waypoint));
  return true;
}

bool
WaypointReaderOzi::VerifyFormat(TLineReader &reader)
{
  const TCHAR *line = reader.ReadLine();
  if (line == nullptr)
    return false;

  return StringStartsWith(StripLeft(line), _T("OziExplorer Waypoint File"));
}
