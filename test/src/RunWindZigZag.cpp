// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Wind/WindZigZag.hpp"
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

  printf("# time quality wind_bearing (deg) wind_speed (m/s) grndspeed (m/s) tas (m/s) bearing (deg)\n");

  WindZigZagGlue wind_zig_zag;

  while (replay->Next()) {
    const MoreData &data = replay->Basic();

    WindZigZagGlue::Result result =
      wind_zig_zag.Update(data, replay->Calculated());
    if (result.quality > 0)
      printf("%d %d %d %g %g %g %d\n", (int)data.time, result.quality,
             (int)result.wind.bearing.Degrees(),
             (double)result.wind.norm,
             (double)data.ground_speed,
             (double)data.true_airspeed,
             (int)data.track.Degrees());
  }

  delete replay;

  return EXIT_SUCCESS;
}

