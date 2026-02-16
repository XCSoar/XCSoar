// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/Settings.hpp"
#include "Computer/CirclingComputer.hpp"
#include "Computer/Wind/WindEKFGlue.hpp"
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

  printf("# time wind_bearing (deg) wind_speed (m/s) grndspeed (m/s) tas (m/s) bearing (deg)\n");

  CirclingSettings circling_settings;
  circling_settings.SetDefaults();

  CirclingComputer circling_computer;
  circling_computer.Reset();

  WindEKFGlue wind_ekf;
  wind_ekf.Reset();

  while (replay->Next()) {
    const MoreData &data = replay->Basic();
    const DerivedInfo &calculated = replay->Calculated();

    circling_computer.TurnRate(replay->SetCalculated(),
                               data, calculated.flight);

    WindEKFGlue::Result result =
      wind_ekf.Update(data, replay->Calculated());
    if (result.quality > 0) {
      char time_buffer[32];
      FormatTime(time_buffer, data.time);

      printf("%s %d %g %g %g %d\n", time_buffer,
               (int)result.wind.bearing.Degrees(),
               (double)result.wind.norm,
               (double)data.ground_speed,
               (double)data.true_airspeed,
               (int)data.track.Degrees());
    }
  }

  delete replay;
}
