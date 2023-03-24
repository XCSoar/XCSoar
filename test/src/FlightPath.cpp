// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/Args.hpp"
#include "DebugReplay.hpp"
#include "Engine/Trace/Trace.hpp"
#include "util/StringCompare.hxx"

using namespace std::chrono;

int main(int argc, char **argv)
{
  TimeStamp start = TimeStamp::Undefined(), end = TimeStamp::Undefined();
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
      start = TimeStamp{FloatDuration{strtol(value, &endptr, 10)}};
      if (endptr == value || *endptr != '\0' || !start.IsDefined()) {
        fputs("The start parameter could not be parsed correctly.\n", stderr);
        args.UsageError();
      }
    } else if ((value = StringAfterPrefix(arg, "--end=")) != nullptr) {
      char *endptr;
      end = TimeStamp{FloatDuration{strtol(value, &endptr, 10)}};
      if (endptr == value || *endptr != '\0' || !end.IsDefined()) {
        fputs("The end parameter could not be parsed correctly.\n", stderr);
        args.UsageError();
      }
    } else {
      args.UsageError();
    }
  }

  if (start.IsDefined() && end.IsDefined() && start >= end) {
    fputs("The start parameter has to be smaller than the end parameter.\n", stderr);
    args.UsageError();
  }

  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == nullptr)
    return EXIT_FAILURE;

  args.ExpectEnd();

  Trace trace({}, Trace::null_time, max_points);

  bool takeoff = false;

  while (replay->Next()) {
    const MoreData &basic = replay->Basic();
    const DerivedInfo &calculated = replay->Calculated();

    if (!basic.time_available || !basic.location_available ||
        !basic.NavAltitudeAvailable())
      continue;

    if (start.IsDefined() && basic.time < start)
      continue;

    if (end.IsDefined() && basic.time > end)
      break;

    trace.push_back(TracePoint(basic));

    if (!start.IsDefined() && calculated.flight.flying && !takeoff) {
      takeoff = true;
      trace.EraseEarlierThan(calculated.flight.takeoff_time);
    }
  }

  delete replay;

  for (auto i = trace.begin(), end = trace.end(); i != end; ++i) {
    const TracePoint &point = *i;
    printf("%u %f %f %d %u\n",
           point.GetTime().count(),
           (double)point.GetLocation().latitude.Degrees(),
           (double)point.GetLocation().longitude.Degrees(),
           point.GetIntegerAltitude(),
           point.GetEngineNoiseLevel());
  }
}
