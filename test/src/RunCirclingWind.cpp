/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Computer/CirclingComputer.hpp"
#include "Computer/Wind/CirclingWind.hpp"
#include "OS/Args.hpp"
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
      TCHAR time_buffer[32];
      FormatTime(time_buffer, replay->Basic().time);

      _tprintf(_T("%s %d %d %g\n"),
               time_buffer, result.quality,
               (int)result.wind.bearing.Degrees(),
               (double)result.wind.norm);
    }
  }
}

