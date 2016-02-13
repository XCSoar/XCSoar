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

#include "OS/Args.hpp"
#include "DebugReplay.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "Util/Macros.hpp"

#include <stdio.h>

static void
LogEvent(const TCHAR *event, double time, const GeoPoint &location)
{
  TCHAR time_buffer[32];
  FormatTime(time_buffer, time);

  _tprintf(_T("%s %s %s\n"), time_buffer,
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
      LogEvent(_T("take-off"), flight.takeoff_time, flight.takeoff_location);
    else if (!flight.flying && last_flying)
      LogEvent(_T("landing"), flight.landing_time, flight.landing_location);
    else if (flight.release_time >= 0 && !last_released)
      LogEvent(_T("release"), flight.release_time, flight.release_location);

    last_flying = flight.flying;
    last_released = flight.release_time >= 0;
  }

  const FlyingState &flight = replay->Calculated().flight;
  if (flight.far_distance >= 0)
    _tprintf(_T("far %u km at %s\n"), unsigned(flight.far_distance / 1000),
             FormatGeoPoint(flight.far_location,
                            CoordinateFormat::DDMMSS).c_str());

  delete replay;
}
