// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugReplay.hpp"
#include "system/Args.hpp"
#include "Computer/WaveComputer.hpp"
#include "Computer/WaveResult.hpp"
#include "Computer/WaveSettings.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "util/Macros.hpp"

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  WaveSettings settings;
  settings.SetDefaults();
  settings.enabled = true;

  WaveComputer wave;
  wave.Reset();

  WaveResult result;
  result.Clear();

  while (replay->Next()) {
    const MoreData &basic = replay->Basic();
    const DerivedInfo &calculated = replay->Calculated();

    wave.Compute(basic, calculated.flight, result, settings);
  }

  delete replay;

  for (const auto &w : result.waves) {
    char time_buffer[32];
    if (w.time.IsDefined())
      FormatTime(time_buffer, w.time);
    else
      strcpy(time_buffer, "?");

    printf("wave: t=%s location=%f,%f a=%f,%f b=%f,%f location=%s normal=%f\n",
             time_buffer,
             (double)w.location.longitude.Degrees(),
             (double)w.location.latitude.Degrees(),
             (double)w.a.longitude.Degrees(),
             (double)w.a.latitude.Degrees(),
             (double)w.b.longitude.Degrees(),
             (double)w.b.latitude.Degrees(),
             FormatGeoPoint(w.location, CoordinateFormat::DDMMSS).c_str(),
             (double)w.normal.Degrees());
  }

  return EXIT_SUCCESS;
}
