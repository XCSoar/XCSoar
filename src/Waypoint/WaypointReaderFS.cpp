// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReaderFS.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Geo/UTM.hpp"
#include "util/StringStrip.hxx"

#include <stdlib.h>

static bool
ParseAngle(const char *src, Angle &angle) noexcept
{
  bool is_positive;
  if (src[0] == 'N' || src[0] == 'n' ||
      src[0] == 'E' || src[0] == 'e')
    is_positive = true;
  else if (src[0] == 'S' || src[0] == 's' ||
           src[0] == 'W' || src[0] == 'w')
    is_positive = false;
  else
    return false;

  char *endptr;

  src++;
  long deg = strtol(src, &endptr, 10);
  if (endptr == src || *endptr != ' ')
    return false;

  src = endptr;
  long min = strtol(src, &endptr, 10);
  if (endptr == src || *endptr != ' ')
    return false;

  src = endptr;
  double sec = strtod(src, &endptr);
  if (endptr == src || *endptr != ' ')
    return false;

  auto value = deg + (double)min / 60 + sec / 3600;
  if (!is_positive)
    value = -value;

  angle = Angle::Degrees(value);
  return true;
}

static bool
ParseLocation(const char *src, GeoPoint &p) noexcept
{
  Angle lon, lat;

  if (!ParseAngle(src, lat))
    return false;

  if (!ParseAngle(src + 17, lon))
    return false;

  p.longitude = lon;
  p.latitude = lat;

  // ensure longitude is within -180:180
  p.Normalize();

  return true;
}

static bool
ParseLocationUTM(const char *src, GeoPoint &p) noexcept
{
  char *endptr;

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

  return true;
}

static bool
ParseAltitude(const char *src, double &dest) noexcept
{
  char *endptr;
  long alt = strtol(src, &endptr, 10);
  if (endptr == src)
    return false;

  dest = alt;
  return true;
}

bool
WaypointReaderFS::ParseLine(const char *line, Waypoints &way_points)
{
  //$FormatGEO
  //ACONCAGU  S 32 39 12.00    W 070 00 42.00  6962  Aconcagua
  //BERGNEUS  N 51 03 07.02    E 007 42 22.02   488  Bergneustadt [A]
  //GOLDENGA  N 37 49 03.00    W 122 28 42.00   227  Golden Gate Bridge
  //REDSQUAR  N 55 45 15.00    E 037 37 12.00   123  Red Square
  //SYDNEYOP  S 33 51 25.02    E 151 12 54.96     5  Sydney Opera

  //$FormatUTM
  //Aconcagu 19H   0405124   6386692   6962  Aconcagua
  //Bergneus 32U   0409312   5656398    488  Bergneustadt [A]
  //Golden G 10S   0545914   4185695    227  Golden Gate Bridge
  //Red Squa 37U   0413390   6179582    123  Red Square
  //Sydney O 56H   0334898   6252272      5  Sydney Opera

  if (line[0] == '\0')
    return true;

  if (line[0] == '$') {
    if (StringStartsWith(line, "$FormatUTM"))
      is_utm = true;
    return true;
  }

  // Determine the length of the line
  size_t len = strlen(line);
  // If less then 27 characters -> something is wrong -> cancel
  if (len < (is_utm ? 39 : 47))
    return false;

  GeoPoint location;
  if (!(is_utm
        ? ParseLocationUTM(line + 9, location)
        : ParseLocation(line + 10, location)))
    return false;

  Waypoint new_waypoint = factory.Create(location);

  new_waypoint.name = std::string{string_converter.Convert({line, 8})};

  if (ParseAltitude(line + (is_utm ? 32 : 41), new_waypoint.elevation))
    new_waypoint.has_elevation = true;
  else
    factory.FallbackElevation(new_waypoint);

  // Description (Characters 35-44)
  if (len > (is_utm ? 38 : 47))
    new_waypoint.comment = std::string{string_converter.Convert(line + (is_utm ? 38 : 47))};

  way_points.Append(std::move(new_waypoint));
  return true;
}

bool
WaypointReaderFS::VerifyFormat(std::string_view contents) noexcept
{
  return contents.starts_with("$FormatUTM") ||
    contents.starts_with("$FormatGEO");
}
