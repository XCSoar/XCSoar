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

#include "Waypoint/WaypointReader.hpp"
#include "Waypoint/Factory.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "OS/ConvertPathName.hpp"
#include "OS/Args.hpp"
#include "Operation/Operation.hpp"

#include <stdint.h>
#include <stdio.h>
#include <tchar.h>

static bool
LoadWaypoints(Path path, Waypoints &waypoints)
{
  NullOperationEnvironment operation;
  if (!ReadWaypointFile(path, waypoints,
                        WaypointFactory(WaypointOrigin::NONE),
                        operation)) {
    fprintf(stderr, "ReadWaypointFile() failed\n");
    return false;
  }

  waypoints.Optimise();
  return true;
}

static bool
ParseGeopoint(const char *line, GeoPoint &location)
{
  double value;
  char *endptr;

  value = strtod(line, &endptr);
  if (line == endptr)
    return false;

  location.latitude = Angle::Degrees(value);
  line = endptr;

  value = strtod(line, &endptr);
  if (line == endptr)
    return false;

  location.longitude = Angle::Degrees(value);

  return true;
}

enum class WaypointType: uint8_t {
  ALL,
  LANDABLE,
  AIRPORT,
};

static bool
AlwaysTrue(const Waypoint &waypoint)
{
  return true;
}

static bool
IsLandable(const Waypoint &waypoint)
{
  return waypoint.IsLandable();
}

static bool
IsAirport(const Waypoint &waypoint)
{
  return waypoint.IsAirport();
}

static WaypointPtr
GetNearestWaypoint(const GeoPoint &location, const Waypoints &waypoints,
                   double range, WaypointType type)
{
  bool (*predicate)(const Waypoint &);
  switch (type) {
  case WaypointType::AIRPORT:
    predicate = IsAirport;
    break;
  case WaypointType::LANDABLE:
    predicate = IsLandable;
    break;
  default:
    predicate = AlwaysTrue;
    break;
  }

  return waypoints.GetNearestIf(location, range, predicate);
}

static void
PrintWaypoint(const Waypoint *waypoint)
{
  if (!waypoint)
    printf("\n");
  else
    _ftprintf(stdout, _T("%f %f %.0f %s\n"),
              (double)waypoint->location.latitude.Degrees(),
              (double)waypoint->location.longitude.Degrees(),
              (double)waypoint->elevation,
              waypoint->name.c_str());
}

int main(int argc, char **argv)
{
  WaypointType type = WaypointType::ALL;
  double range = 100000;

  Args args(argc, argv,
            "PATH\n\nPATH is expected to be any compatible waypoint file.\n"
            "Stdin expects a list of coordinates at floating point values\n"
            "in the format: LAT LON\n\ne.g.\n"
            "-23.49858 123.45838\n"
            "2.12343 34.38432\n"
            "65.18234 -173.48307\n\n"
            "Output is in the format: LAT LON ELEV (in m) NAME\n\ne.g.\n"
            "50.823055 6.186384 189 Aachen Merzbruc");

  const char *arg;
  while ((arg = args.PeekNext()) != NULL && *arg == '-') {
    args.Skip();

    const char *value;
    if ((value = StringAfterPrefix(arg, "--range=")) != NULL) {
      double _range = strtod(value, NULL);
      if (_range > 0)
        range = _range;
    } else if (StringStartsWith(arg, "--airports-only")) {
      type = WaypointType::AIRPORT;
    } else if (StringStartsWith(arg, "--landables-only")) {
      type = WaypointType::LANDABLE;
    } else {
      args.UsageError();
    }
  }

  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  Waypoints waypoints;
  if (!LoadWaypoints(path, waypoints))
    return EXIT_FAILURE;

  char buffer[1024];
  const char *line;
  while ((line = fgets(buffer, sizeof(buffer) - 3, stdin)) != NULL) {
    GeoPoint location;
    if (!ParseGeopoint(line, location))
      continue;

    const auto waypoint = GetNearestWaypoint(location, waypoints,
                                             range, type);
    PrintWaypoint(waypoint.get());
  }

  return EXIT_SUCCESS;
}
