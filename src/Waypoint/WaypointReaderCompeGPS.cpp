// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReaderCompeGPS.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Geo/UTM.hpp"
#include "util/StringSplit.hxx"

static bool
ParseAngle(const char *&src, Angle &angle) noexcept
{
  // 41.234234N

  char *endptr;

  // Parse numerical value
  double value = strtod(src, &endptr);
  if (endptr == src)
    return false;

  src = endptr;
  angle = Angle::Degrees(value);

  // Skip until next whitespace and look for NSEW signs
  bool found = false;
  while (*src != ' ' && *src != '\0') {
    if (!found) {
      if (*src == 'N' || *src == 'n' ||
          *src == 'E' || *src == 'e') {
        found = true;
      } else if (*src == 'S' || *src == 's' ||
                 *src == 'W' || *src == 'w') {
        found = true;
        angle.Flip();
      }
    }

    src++;
  }

  return found;
}

static bool
ParseLocation(const char *&src, GeoPoint &p) noexcept
{
  // A 41.234234N 7.234424W

  // Ignore but require 'A' placeholder
  if (*src != 'A')
    return false;

  src++;

  // Skip whitespace
  while (*src == ' ')
    src++;

  Angle lat, lon;
  if (!ParseAngle(src, lat) || !ParseAngle(src, lon))
    return false;

  p.longitude = lon;
  p.latitude = lat;

  // ensure longitude is within -180:180
  p.Normalize();

  return true;
}

static bool
ParseLocationUTM(const char *&src, GeoPoint &p) noexcept
{
  // 31T 318570 4657569

  char *endptr;

  // Parse zone number
  long zone_number = strtol(src, &endptr, 10);
  if (endptr == src)
    return false;

  src = endptr;
  char zone_letter = src[0];

  src++;
  long easting = strtol(src, &endptr, 10);
  if (endptr == src || *endptr != ' ')
    return false;

  src = endptr;
  long northing = strtol(src, &endptr, 10);
  if (endptr == src || *endptr != ' ')
    return false;

  UTM u(zone_number, zone_letter, easting, northing);
  p = u.ToGeoPoint();

  // ensure longitude is within -180:180
  p.Normalize();

  src = endptr;

  return true;
}

static bool
ParseAltitude(const char *&src, double &dest) noexcept
{
  char *endptr;
  double value = strtod(src, &endptr);
  if (endptr == src)
    return false;

  dest = value;
  src = endptr;
  return true;
}

bool
WaypointReaderCompeGPS::ParseLine(const char *line, Waypoints &waypoints)
{
  /*
   * G  WGS 84
   * U  1
   * W  IT05FC A 46.9121939503ºN 11.9605922700°E 27-MAR-62 00:00:00 566.000000 Ahornach Sand, Ahornach LP, GS und HG
   * w  Waypoint,0,-1.0,16777215,255,0,0,7,,0.0,
   * W  IT05FB A 46.9260440931ºN 11.9676733017°E 27-MAR-62 00:00:00 1425.000000 Ahornach Sand, Ahornach SP, GS und HG
   * w  Waypoint,0,-1.0,16777215,255,0,0,7,,0.0,
   *
   * W ShortName 31T 318570 4657569 27-MAR-62 00:00:00 0 some Comments
   * W ShortName A 41.234234N 7.234424W 27-MAR-62 00:00:00 0 Comments
   */

  // Skip projection and file encoding information
  if (*line == 'G' || *line == 'B')
    return true;

  // Check for format: UTM or LatLon
  if (StringStartsWith(line, "U  0")) {
    is_utm = true;
    return true;
  }

  // Skip non-waypoint lines
  if (*line != 'W')
    return true;

  // Skip W indicator and whitespace
  line++;
  while (*line == ' ')
    line++;

  // Find next space delimiter, skip shortname
  const char *name = line;
  const char *space = strstr(line, " ");
  if (space == nullptr)
    return false;

  unsigned name_length = space - line;
  if (name_length == 0)
    return false;

  line = space;
  while (*line == ' ')
    line++;

  // Parse location
  GeoPoint location;
  if (!(is_utm
        ? ParseLocationUTM(line, location)
        : ParseLocation(line, location)))
    return false;

  // Skip whitespace
  while (*line == ' ')
    line++;

  // Skip unused date field
  line = strstr(line, " ");
  if (line == nullptr)
    return false;

  line++;

  // Skip unused time field
  line = strstr(line, " ");
  if (line == nullptr)
    return false;

  line++;

  // Create new waypoint instance
  Waypoint waypoint = factory.Create(location);
  waypoint.name.assign(string_converter.Convert({name, name_length}));

  // Parse altitude
  if (ParseAltitude(line, waypoint.elevation))
    waypoint.has_elevation = true;
  else
    factory.FallbackElevation(waypoint);

  // Skip whitespace
  while (*line == ' ')
    line++;

  // Parse waypoint name
  waypoint.comment.assign(string_converter.Convert(line));

  waypoints.Append(std::move(waypoint));
  return true;
}

bool
WaypointReaderCompeGPS::VerifyFormat(std::string_view contents) noexcept
{
  // Ignore optional line with encoding information
  if (contents.starts_with("B "))
    contents = Split(contents, '\n').second;

  return contents.starts_with("G  WGS 84");
}
