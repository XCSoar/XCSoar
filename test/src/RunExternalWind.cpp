// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugReplay.hpp"
#include "system/Args.hpp"
#include "Formatter/TimeFormatter.hpp"

#include <stdio.h>
#include <memory>

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  std::unique_ptr<DebugReplay> replay(CreateDebugReplay(args));
  if (!replay)
    return EXIT_FAILURE;

  args.ExpectEnd();

  printf("# time wind_bearing (deg) wind_speed (m/s)\n");

  Validity last_available;
  last_available.Clear();

  while (replay->Next()) {
    const auto &basic = replay->Basic();
    if (basic.external_wind_available.Modified(last_available)) {
      last_available = basic.external_wind_available;

      char time_buffer[32];
      FormatTime(time_buffer, basic.time);

      printf("%s %d %g\n",
               time_buffer,
               (int)basic.external_wind.bearing.Degrees(),
               (double)basic.external_wind.norm);
    }
  }
}
