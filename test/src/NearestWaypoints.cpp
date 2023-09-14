// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Waypoint/WaypointReader.hpp"
#include "Waypoint/Factory.hpp"
#include "Waypoint/Waypoints.hpp"
#include "system/ConvertPathName.hpp"
#include "system/Args.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "util/PrintException.hxx"

#include <cstdint>
#include <stdio.h>
#include <tchar.h>

static void
LoadWaypoints(Path path, Waypoints &waypoints)
{
  ConsoleOperationEnvironment operation;
  ReadWaypointFile(path, waypoints,
                   WaypointFactory(WaypointOrigin::NONE),
                   operation);
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
AlwaysTrue([[maybe_unused]] const Waypoint &waypoint)
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
              (double)waypoint->GetElevationOrZero(),
              waypoint->name.c_str());
}

int main(int argc, char **argv)
try {
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
  LoadWaypoints(path, waypoints);

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
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
