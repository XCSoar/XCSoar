// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/ConvertPathName.hpp"
#include "IGC/IGCWriter.hpp"
#include "time/GPSClock.hpp"
#include "DebugReplay.hpp"
#include "system/Args.hpp"

#include <stdio.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER INFILE OUTFILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  const auto output_file = args.ExpectNextPath();
  args.ExpectEnd();

  while (!replay->Basic().time_available)
    if (!replay->Next())
      return 0;

  const char *driver_name = _T("Unknown");

  IGCWriter writer(output_file);
  writer.WriteHeader(replay->Basic().date_time_utc, _T("Manfred Mustermann"), _T("Manuela Mustermann"),
                     _T("Ventus"), _T("D-1234"),
                     _T("MM"), "FOO", driver_name, true);

  GPSClock log_clock;
  while (replay->Next())
    if (log_clock.CheckAdvance(replay->Basic().time, std::chrono::seconds(1)))
      writer.LogPoint(replay->Basic());

  writer.Flush();

  delete replay;

  return EXIT_SUCCESS;
}
