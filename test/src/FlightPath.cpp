/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

int main(int argc, char **argv)
{
  unsigned max_points = 1000;

  Args args(argc, argv, "[--max-points=1000] DRIVER FILE");

  const char *arg;
  while ((arg = args.PeekNext()) != NULL && *arg == '-') {
    args.Skip();

    const char *value;
    if ((value = StringAfterPrefix(arg, "--max-points=")) != NULL) {
      unsigned _max_points = strtol(value, NULL, 10);
      if (_max_points > 0)
        max_points = _max_points;
    } else {
      args.UsageError();
    }
  }

  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  Trace trace(0, Trace::null_time, max_points);

  bool takeoff = false;

  while (replay->Next()) {
    const MoreData &basic = replay->Basic();
    const DerivedInfo &calculated = replay->Calculated();

    if (!basic.time_available || !basic.location_available ||
        !basic.NavAltitudeAvailable())
      continue;

    trace.push_back(TracePoint(basic));

    if (calculated.flight.flying && !takeoff) {
      takeoff = true;
      trace.EraseEarlierThan(calculated.flight.takeoff_time);
    }
  }

  delete replay;

  for (auto i = trace.begin(), end = trace.end(); i != end; ++i) {
    const TracePoint &point = *i;
    printf("%u %f %f %d %u\n",
           point.GetTime(),
           (double)point.GetLocation().latitude.Degrees(),
           (double)point.GetLocation().longitude.Degrees(),
           point.GetIntegerAltitude(),
           point.GetEngineNoiseLevel());
  }
}
