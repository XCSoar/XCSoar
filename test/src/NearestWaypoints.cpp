/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Waypoint/Waypoints.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "OS/PathName.hpp"
#include "OS/Args.hpp"
#include "Operation/Operation.hpp"

#include <stdio.h>
#include <tchar.h>

static bool
LoadWaypoints(const char *_path, Waypoints &waypoints)
{
  PathName path(_path);
  WaypointReader parser(path, 0);
  if (parser.Error()) {
    fprintf(stderr, "WayPointParser::SetFile() has failed\n");
    return false;
  }

  NullOperationEnvironment operation;
  if (!parser.Parse(waypoints, operation)) {
    fprintf(stderr, "WayPointParser::Parse() has failed\n");
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

  location.latitude = Angle::Degrees(fixed(value));
  line = endptr;

  value = strtod(line, &endptr);
  if (line == endptr)
    return false;

  location.longitude = Angle::Degrees(fixed(value));

  return true;
}

static const Waypoint *
GetNearestWaypoint(const GeoPoint &location, const Waypoints &waypoints,
                   fixed range)
{
  return waypoints.GetNearest(location, range);
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
  fixed range = fixed_int_constant(100000);

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
        range = fixed(_range);
    } else {
      args.UsageError();
    }
  }

  const char *path = args.ExpectNext();
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

    const Waypoint *waypoint = GetNearestWaypoint(location, waypoints, range);
    PrintWaypoint(waypoint);
  }

  return EXIT_SUCCESS;
}
