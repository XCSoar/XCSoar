// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/CirclingComputer.hpp"
#include "Computer/Wind/CirclingWind.hpp"
#include "system/Args.hpp"
#include "DebugReplay.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Computer/Settings.hpp"

#include <stdio.h>
#include <memory>

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  std::unique_ptr<DebugReplay> replay(CreateDebugReplay(args));
  if (!replay)
    return EXIT_FAILURE;

  args.ExpectEnd();

  printf("# time quality wind_bearing (deg) wind_speed (m/s)\n");

  CirclingSettings circling_settings;
  circling_settings.SetDefaults();

  CirclingComputer circling_computer;
  circling_computer.Reset();

  CirclingWind circling_wind;
  circling_wind.Reset();

  while (replay->Next()) {
    circling_computer.TurnRate(replay->SetCalculated(),
                               replay->Basic(),
                               replay->Calculated().flight);
    circling_computer.Turning(replay->SetCalculated(),
                              replay->Basic(),
                              replay->Calculated().flight,
                              circling_settings);

    CirclingWind::Result result = circling_wind.NewSample(replay->Basic(),
                                                          replay->Calculated());
    if (result.quality > 0) {
      char time_buffer[32];
      FormatTime(time_buffer, replay->Basic().time);

      _tprintf(_T("%s %d %d %g\n"),
               time_buffer, result.quality,
               (int)result.wind.bearing.Degrees(),
               (double)result.wind.norm);
    }
  }
}

