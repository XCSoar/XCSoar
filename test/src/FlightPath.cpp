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

#include "OS/Args.hpp"
#include "DebugReplay.hpp"
#include "Engine/Trace/Trace.hpp"
#include "NMEA/Aircraft.hpp"

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  const unsigned max_points = 1000;
  Trace trace(0, Trace::null_time, max_points);

  while (replay->Next()) {
    const MoreData &basic = replay->Basic();
    const DerivedInfo &calculated = replay->Calculated();

    if (!basic.time_available || !basic.location_available ||
        !basic.NavAltitudeAvailable() ||
        !calculated.flight.flying)
      continue;

    const AircraftState state = ToAircraftState(basic, calculated);
    trace.append(state);
  }

  delete replay;

  for (auto i = trace.begin(), end = trace.end(); i != end; ++i) {
    const TracePoint &point = *i;
    printf("%u %f %f %d\n",
           point.GetTime(),
           (double)point.get_location().latitude.Degrees(),
           (double)point.get_location().longitude.Degrees(),
           point.GetIntegerAltitude());
  }
}
