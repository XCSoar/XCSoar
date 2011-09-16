/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "OS/PathName.hpp"
#include "Logger/IGCWriter.hpp"
#include "DebugReplay.hpp"
#include "Args.hpp"

#include <stdio.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER INFILE OUTFILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  const char *output_file = args.ExpectNext();
  args.ExpectEnd();

  while (!replay->Basic().time_available)
    if (!replay->Next())
      return 0;

  const TCHAR *driver_name = _T("Unknown");

  PathName igc_path(output_file);
  IGCWriter writer(igc_path, replay->Basic());
  writer.header(replay->Basic().date_time_utc,
                _T("Manfred Mustermann"), _T("Ventus"),
                _T("D-1234"), _T("Foo"), driver_name);

  GPSClock log_clock(fixed(1));
  while (replay->Next())
    if (log_clock.check_advance(replay->Basic().time))
      writer.LogPoint(replay->Basic());

  writer.flush();

  return EXIT_SUCCESS;
}
