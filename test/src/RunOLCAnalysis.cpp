/* Copyright_License {

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

#include "Engine/Trace/Trace.hpp"
#include "Contest/ContestManager.hpp"
#include "Printing.hpp"
#include "OS/Args.hpp"
#include "DebugReplay.hpp"

#include <assert.h>
#include <stdio.h>

// Uncomment the following line to use the same trace size as LK8000.
//#define BENCHMARK_LK8000

#ifdef BENCHMARK_LK8000
static Trace full_trace(0, Trace::null_time, 100);
static Trace triangle_trace(0, Trace::null_time, 100);
static Trace sprint_trace(0, 9000, 50);
#else
static Trace full_trace(0, Trace::null_time, 512);
static Trace triangle_trace(0, Trace::null_time, 1024);
static Trace sprint_trace(0, 9000, 128);
#endif

static ContestManager olc_classic(Contest::OLC_CLASSIC,
                                  full_trace, triangle_trace, sprint_trace);
static ContestManager olc_fai(Contest::OLC_FAI,
                              full_trace, triangle_trace, sprint_trace);
static ContestManager olc_sprint(Contest::OLC_SPRINT,
                                 full_trace, triangle_trace, sprint_trace);
static ContestManager olc_league(Contest::OLC_LEAGUE,
                                 full_trace, triangle_trace, sprint_trace);
static ContestManager olc_plus(Contest::OLC_PLUS,
                               full_trace, triangle_trace, sprint_trace);
static ContestManager dmst(Contest::DMST,
                           full_trace, triangle_trace, sprint_trace);
static ContestManager xcontest(Contest::XCONTEST,
                               full_trace, triangle_trace, sprint_trace);
static ContestManager sis_at(Contest::SIS_AT,
                             full_trace, triangle_trace, sprint_trace);
static ContestManager olc_netcoupe(Contest::NET_COUPE,
                                   full_trace, triangle_trace, sprint_trace);

static int
TestOLC(DebugReplay &replay)
{
  bool released = false;

  for (int i = 1; replay.Next(); i++) {
    if (i % 500 == 0) {
      putchar('.');
      fflush(stdout);
    }

    const MoreData &basic = replay.Basic();
    if (!basic.time_available || !basic.location_available ||
        !basic.NavAltitudeAvailable())
      continue;

    if (!released && replay.Calculated().flight.release_time >= 0) {
      released = true;

      triangle_trace.EraseEarlierThan(replay.Calculated().flight.release_time);
      full_trace.EraseEarlierThan(replay.Calculated().flight.release_time);
      sprint_trace.EraseEarlierThan(replay.Calculated().flight.release_time);
    }

    const TracePoint point(basic);
    triangle_trace.push_back(point);
    full_trace.push_back(point);
    sprint_trace.push_back(point);

    olc_sprint.UpdateIdle();
    olc_league.UpdateIdle();
  }

  olc_classic.SolveExhaustive();
  olc_fai.SolveExhaustive();
  olc_league.SolveExhaustive();
  olc_plus.SolveExhaustive();
  dmst.SolveExhaustive();
  xcontest.SolveExhaustive();
  sis_at.SolveExhaustive();
  olc_netcoupe.SolveExhaustive();

  putchar('\n');

  std::cout << "classic\n";
  PrintHelper::print(olc_classic.GetStats().GetResult());
  std::cout << "league\n";
  std::cout << "# league\n";
  PrintHelper::print(olc_league.GetStats().GetResult(0));
  std::cout << "# classic\n";
  PrintHelper::print(olc_league.GetStats().GetResult(1));
  std::cout << "fai\n";
  PrintHelper::print(olc_fai.GetStats().GetResult());
  std::cout << "sprint\n";
  PrintHelper::print(olc_sprint.GetStats().GetResult());
  std::cout << "plus\n";
  std::cout << "# classic\n";
  PrintHelper::print(olc_plus.GetStats().GetResult(0));
  std::cout << "# triangle\n";
  PrintHelper::print(olc_plus.GetStats().GetResult(1));
  std::cout << "# plus\n";
  PrintHelper::print(olc_plus.GetStats().GetResult(2));

  std::cout << "dmst\n";
  PrintHelper::print(dmst.GetStats().GetResult());

  std::cout << "xcontest\n";
  std::cout << "# free\n";
  PrintHelper::print(xcontest.GetStats().GetResult(0));
  std::cout << "# triangle\n";
  PrintHelper::print(xcontest.GetStats().GetResult(1));

  std::cout << "sis_at\n";
  PrintHelper::print(sis_at.GetStats().GetResult(0));

  std::cout << "netcoupe\n";
  PrintHelper::print(olc_netcoupe.GetStats().GetResult());

  olc_classic.Reset();
  olc_fai.Reset();
  olc_sprint.Reset();
  olc_league.Reset();
  olc_plus.Reset();
  dmst.Reset();
  olc_netcoupe.Reset();
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
