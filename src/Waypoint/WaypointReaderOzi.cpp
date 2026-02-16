// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReaderOzi.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Units/System.hpp"
#include "util/NumberParser.hxx"
#include "util/StringSplit.hxx"
#include "util/StringStrip.hxx"

#include <stdlib.h>

static bool
ParseAngle(const char *src, Angle &angle) noexcept
{
  char *endptr;
  double deg = strtod(src, &endptr);
  if (endptr == src)
    return false;

  angle = Angle::Degrees(deg);
  return true;
}

static std::string_view
NextColumn(std::string_view &line) noexcept
{
  auto [value, rest] = Split(line, ',');
  line = rest;
  return Strip(value);
}

bool
WaypointReaderOzi::ParseLine(const char *line, Waypoints &way_points)
{
  if (line[0] == '\0')
    return true;

  // Ignore first four header lines
  if (ignore_lines > 0) {
    --ignore_lines;
    return true;
  }

  std::string_view rest{line};

  // Field 1 : Number
  unsigned number = 0;
  ParseIntegerTo(NextColumn(rest), number);

  // Field 2 : Name
  std::string name{string_converter.Convert(NextColumn(rest))};

  GeoPoint location;
  // Latitude (e.g. 5115.900N)
  if (!ParseAngle(NarrowString<40>(NextColumn(rest)), location.latitude))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!ParseAngle(NarrowString<40>(NextColumn(rest)), location.longitude))
    return false;

  location.Normalize(); // ensure longitude is within -180:180

  NextColumn(rest); // Field 5 : Date
  NextColumn(rest); // Field 6 : Symbol
  NextColumn(rest); // Field 7 : Status
  NextColumn(rest); // Field 8 : Map Display
  NextColumn(rest); // Field 9 : Foreground Color
  NextColumn(rest); // Field 10 : Background Color

  // Field 11 : Description
  std::string comment{string_converter.Convert(NextColumn(rest))};

  NextColumn(rest); // Field 12 : Pointer Direction
  NextColumn(rest); // Field 13 : Garmin Display Format
  NextColumn(rest); // Field 14 : Proximity Distance

  // Field 15 : Altitude - in feet (-777 if not valid)
  auto elevation_feet = ParseInteger<int>(NextColumn(rest));
  if (elevation_feet && *elevation_feet == -777)
    elevation_feet.reset();

  Waypoint new_waypoint = factory.Create(location);

  new_waypoint.original_id = number;
  new_waypoint.name = std::move(name);
  new_waypoint.comment = std::move(comment);

  if (elevation_feet) {
    new_waypoint.elevation = Units::ToSysUnit(*elevation_feet, Unit::FEET);
    new_waypoint.has_elevation = true;
  } else
    factory.FallbackElevation(new_waypoint);

  way_points.Append(std::move(new_waypoint));
  return true;
}

bool
WaypointReaderOzi::VerifyFormat(std::string_view contents) noexcept
{
  return contents.starts_with("OziExplorer Waypoint File");
}
