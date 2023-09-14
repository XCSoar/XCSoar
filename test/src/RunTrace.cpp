// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/Args.hpp"
#include "DebugReplay.hpp"
#include "Engine/Trace/Trace.hpp"

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  Trace trace;

  while (replay->Next()) {
    const MoreData &basic = replay->Basic();
    if (basic.time_available && basic.location_available &&
        basic.NavAltitudeAvailable())
      trace.push_back(TracePoint(basic));
  }

  delete replay;
}
