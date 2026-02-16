// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/Wind/Computer.hpp"
#include "Computer/CirclingComputer.hpp"
#include "Computer/Settings.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "system/Args.hpp"
#include "DebugReplay.hpp"

#include <stdio.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  printf("# time wind_bearing (deg) wind_speed (m/s)\n");

  GlidePolar glide_polar(0);

  CirclingSettings circling_settings;

  WindSettings wind_settings;
  wind_settings.SetDefaults();

  CirclingComputer circling_computer;
  circling_computer.Reset();

  WindComputer wind_computer;
  wind_computer.Reset();

  Validity last;
  last.Clear();

  while (replay->Next()) {
    const MoreData &basic = replay->Basic();
    const DerivedInfo &calculated = replay->Calculated();

    circling_computer.TurnRate(replay->SetCalculated(),
                               basic, calculated.flight);
    circling_computer.Turning(replay->SetCalculated(),
                              basic,
                              calculated.flight,
                              circling_settings);

    wind_computer.Compute(wind_settings, glide_polar, basic,
                          replay->SetCalculated());

    if (calculated.estimated_wind_available.Modified(last)) {
      char time_buffer[32];
      FormatTime(time_buffer, replay->Basic().time);

      _tprintf(_T("%s %d %g\n"),
               time_buffer, (int)calculated.estimated_wind.bearing.Degrees(),
               (double)calculated.estimated_wind.norm);
    }

    last = calculated.estimated_wind_available;
  }

  delete replay;
}
