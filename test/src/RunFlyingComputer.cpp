// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/Args.hpp"
#include "DebugReplay.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "util/Macros.hpp"

#include <stdio.h>

static void
LogEvent(const char *event, TimeStamp time, const GeoPoint &location) noexcept
{
  char time_buffer[32];
  FormatTime(time_buffer, time);

  printf("%s %s %s\n", time_buffer,
           FormatGeoPoint(location, CoordinateFormat::DDMMSS).c_str(),
           event);
}

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  bool last_flying = false, last_released = false;

  while (replay->Next()) {
    const FlyingState &flight = replay->Calculated().flight;

    if (flight.flying && !last_flying)
      LogEvent("take-off", flight.takeoff_time, flight.takeoff_location);
    else if (!flight.flying && last_flying)
      LogEvent("landing", flight.landing_time, flight.landing_location);
    else if (flight.release_time.IsDefined() && !last_released)
      LogEvent("release", flight.release_time, flight.release_location);

    last_flying = flight.flying;
    last_released = flight.release_time.IsDefined();
  }

  const FlyingState &flight = replay->Calculated().flight;
  if (flight.far_distance >= 0)
    printf("far %u km at %s\n", unsigned(flight.far_distance / 1000),
             FormatGeoPoint(flight.far_location,
                            CoordinateFormat::DDMMSS).c_str());

  delete replay;
}
