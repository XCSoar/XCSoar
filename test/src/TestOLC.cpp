/* Copyright_License {

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

#include "Replay/IGCParser.hpp"
#include "IO/FileLineReader.hpp"
#include "Engine/Math/Earth.hpp"
#include "Engine/Trace/Trace.hpp"
#include "Engine/Task/Tasks/ContestManager.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Printing.hpp"

#include <assert.h>
#include <stdio.h>

// Uncomment the following line to use the same trace size as LK8000.
//#define MATEUSZ_IS_A_CHEATER

#ifdef MATEUSZ_IS_A_CHEATER
static Trace full_trace(60, Trace::null_time, 100);
static Trace sprint_trace(0, 9000, 50);
#else
static Trace full_trace(60);
static Trace sprint_trace(0, 9000, 300);
#endif

ContestManager olc_classic(OLC_Classic, full_trace, sprint_trace);
ContestManager olc_fai(OLC_FAI, full_trace, sprint_trace);
ContestManager olc_sprint(OLC_Sprint, full_trace, sprint_trace);
ContestManager olc_league(OLC_League, full_trace, sprint_trace);
ContestManager olc_plus(OLC_Plus, full_trace, sprint_trace);

static void
on_advance(const GeoPoint &loc, const fixed speed,
           const Angle bearing, const fixed alt,
           const fixed baroalt, const fixed t)
{
  AircraftState new_state;
  new_state.location = loc;
  new_state.ground_speed = speed;
  new_state.altitude = alt;
  new_state.track = bearing;
  new_state.time = t;
  new_state.altitude_agl = alt;

  full_trace.append(new_state);
  sprint_trace.append(new_state);

  full_trace.optimise_if_old();
  sprint_trace.optimise_if_old();
}

static int
TestOLC(const char *filename)
{
  FileLineReaderA reader(filename);
  if (reader.error()) {
    fprintf(stderr, "Failed to open %s\n", filename);
    return EXIT_FAILURE;
  }

  char *line;
  for (int i = 1; (line = reader.read()) != NULL; i++) {
    if (i % 500 == 0) {
      putchar('.');
      fflush(stdout);
    }

    IGCFix fix;
    if (!IGCParseFix(line, fix))
      continue;

    on_advance(fix.location, fixed(30), Angle::zero(),
               fix.gps_altitude, fix.pressure_altitude,
               fix.time);

    olc_sprint.update_idle();
  }

  olc_classic.solve_exhaustive();
  olc_fai.solve_exhaustive();
  olc_league.solve_exhaustive();
  olc_plus.solve_exhaustive();

  putchar('\n');

  std::cout << "classic\n";
  PrintHelper::print(olc_classic.get_stats().get_contest_result());
  std::cout << "league\n";
  PrintHelper::print(olc_league.get_stats().get_contest_result());
  std::cout << "fai\n";
  PrintHelper::print(olc_fai.get_stats().get_contest_result());
  std::cout << "sprint\n";
  PrintHelper::print(olc_sprint.get_stats().get_contest_result());
  std::cout << "plus\n";
  PrintHelper::print(olc_plus.get_stats().get_contest_result());

  olc_classic.reset();
  olc_fai.reset();
  olc_sprint.reset();
  olc_league.reset();
  olc_plus.reset();
  full_trace.clear();
  sprint_trace.clear();

  return 0;
}


int main(int argc, char **argv)
{
  assert(argc >= 2);

  return TestOLC(argv[1]);
}
