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
#include "Engine/Trace/Trace.hpp"
#include "Util/StringCompare.hxx"

int main(int argc, char **argv)
{
  int start = -1, end = -1;
  unsigned max_points = 1000;

  Args args(argc, argv,
            "[options] DRIVER FILE\n"
            "Options:\n"
            "  --start=5000             Begin flight path at 5000 sec after UTC midnight,\n"
            "                           if not defined takeoff time is used\n"
            "  --end=15000              End flight path at 15000 sec after UTC midnight,\n"
            "                           if not defined the last timestamp in the file is used\n"
            "  --max-points=1000        Maximum number of trace points in output (default = 1000)");

  const char *arg;
  while ((arg = args.PeekNext()) != nullptr && *arg == '-') {
    args.Skip();

    const char *value;
    if ((value = StringAfterPrefix(arg, "--max-points=")) != nullptr) {
      unsigned _max_points = strtol(value, NULL, 10);
      if (_max_points > 0)
        max_points = _max_points;
    } else if ((value = StringAfterPrefix(arg, "--start=")) != nullptr) {
      char *endptr;
      start = strtol(value, &endptr, 10);
      if (endptr == value || *endptr != '\0' || start < 0) {
        fputs("The start parameter could not be parsed correctly.\n", stderr);
        args.UsageError();
      }
    } else if ((value = StringAfterPrefix(arg, "--end=")) != nullptr) {
      char *endptr;
      end = strtol(value, &endptr, 10);
      if (endptr == value || *endptr != '\0' || end < 0) {
        fputs("The end parameter could not be parsed correctly.\n", stderr);
        args.UsageError();
      }
    } else {
      args.UsageError();
    }
  }

  if (start >= 0 && end >= 0 && start >= end) {
    fputs("The start parameter has to be smaller than the end parameter.\n", stderr);
    args.UsageError();
  }

  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == nullptr)
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

    if (start >= 0 && (int)basic.time < start)
      continue;

    if (end >= 0 && (int)basic.time > end)
      break;

    trace.push_back(TracePoint(basic));

    if (start < 0 && calculated.flight.flying && !takeoff) {
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
