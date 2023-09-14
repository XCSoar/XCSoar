// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReaderOzi.hpp"
#include "Waypoint/Waypoints.hpp"
#include "io/LineReader.hpp"
#include "Units/System.hpp"
#include "util/Macros.hpp"
#include "util/ExtractParameters.hpp"
#include "util/StringStrip.hxx"

#include <stdlib.h>

static bool
ParseAngle(const TCHAR *src, Angle &angle) noexcept
{
  TCHAR *endptr;
  double deg = _tcstod(src, &endptr);
  if (endptr == src)
    return false;

  angle = Angle::Degrees(deg);
  return true;
}

static bool
ParseNumber(const TCHAR *src, long &dest) noexcept
{
  TCHAR *endptr;
  long temp = _tcstol(src, &endptr, 10);
  if (endptr == src)
    return false;

  dest = temp;
  return true;
}

static bool
ParseString(tstring_view src, tstring &dest) noexcept
{
  if (src.empty())
    return false;

  dest.assign(Strip(src));
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

  if (ParseNumber(params[14], value) && value != -777) {
    new_waypoint.elevation = Units::ToSysUnit(value, Unit::FEET);
    new_waypoint.has_elevation = true;
  } else
    factory.FallbackElevation(new_waypoint);

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
