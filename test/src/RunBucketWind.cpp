// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/Wind/BucketWind.hpp"
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

  printf("# time wind_bearing (deg) wind_speed (m/s) calculated_TAS instrument_TAS N_BINS\n");

  const MoreData &data = replay->Basic();
  BucketWind bucket_wind;
  bucket_wind.Reset();

  while (replay->Next()) {
    if (data.time.IsDefined() && data.track_available && data.ground_speed_available) {
      bucket_wind.Update(data.time, data.track, data.ground_speed);

      BucketWind::Result result = bucket_wind.Fit(replay->Basic().time);
      if (result.valid) {
        char time_buffer[32];
        FormatTime(time_buffer, replay->Basic().time);

        printf("%s %d %g %g %g %d\n",
                 time_buffer,
                 (int)result.wind.bearing.Degrees(),
                 (double)result.wind.norm,
                 (double)result.tas,
                 (double)data.true_airspeed,
                 (int)result.n_bins);
      }
    }
  }
}

