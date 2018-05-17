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

#include "Wind/WindZigZag.hpp"
#include "OS/Args.hpp"
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

