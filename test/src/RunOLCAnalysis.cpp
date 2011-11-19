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

#include "Engine/Math/Earth.hpp"
#include "Engine/Trace/Trace.hpp"
#include "Contest/ContestManager.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Printing.hpp"
#include "Args.hpp"
#include "DebugReplay.hpp"
#include "NMEA/Aircraft.hpp"

#include <assert.h>
#include <stdio.h>

// Uncomment the following line to use the same trace size as LK8000.
//#define BENCHMARK_LK8000

#ifdef BENCHMARK_LK8000
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

static int
TestOLC(DebugReplay &replay)
{
  for (int i = 1; replay.Next(); i++) {
    if (i % 500 == 0) {
      putchar('.');
      fflush(stdout);
    }

    const AircraftState state =
      ToAircraftState(replay.Basic(), replay.Calculated());
    full_trace.append(state);
    sprint_trace.append(state);

    full_trace.optimise_if_old();
    sprint_trace.optimise_if_old();

    olc_sprint.UpdateIdle();
  }

  olc_classic.SolveExhaustive();
  olc_fai.SolveExhaustive();
  olc_league.SolveExhaustive();
  olc_plus.SolveExhaustive();

  putchar('\n');

  std::cout << "classic\n";
  PrintHelper::print(olc_classic.GetStats().GetResult());
  std::cout << "league\n";
  PrintHelper::print(olc_league.GetStats().GetResult());
  std::cout << "fai\n";
  PrintHelper::print(olc_fai.GetStats().GetResult());
  std::cout << "sprint\n";
  PrintHelper::print(olc_sprint.GetStats().GetResult());
  std::cout << "plus\n";
  PrintHelper::print(olc_plus.GetStats().GetResult());

  olc_classic.Reset();
  olc_fai.Reset();
  olc_sprint.Reset();
  olc_league.Reset();
  olc_plus.Reset();
  full_trace.clear();
  sprint_trace.clear();

  return 0;
}


int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  int result = TestOLC(*replay);
  delete replay;
  return result;
}
