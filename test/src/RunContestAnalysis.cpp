// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Trace/Trace.hpp"
#include "Contest/ContestManager.hpp"
#include "Printing.hpp"
#include "system/Args.hpp"
#include "DebugReplay.hpp"

#include <cassert>
#include <stdio.h>

using namespace std::chrono;

// Uncomment the following line to use the same trace size as LK8000.
//#define BENCHMARK_LK8000

#ifdef BENCHMARK_LK8000
static Trace full_trace({}, Trace::null_time, 100);
static Trace triangle_trace({}, Trace::null_time, 100);
static Trace sprint_trace({}, minutes{120}, 50);
#else
static Trace full_trace({}, Trace::null_time, 512);
static Trace triangle_trace({}, Trace::null_time, 1024);
static Trace sprint_trace({}, minutes{120}, 128);
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
static ContestManager weglide_free(Contest::WEGLIDE_FREE,
                               full_trace, triangle_trace, sprint_trace);
static ContestManager charron(Contest::CHARRON,
                              full_trace, triangle_trace, sprint_trace);

static int
TestContest(DebugReplay &replay)
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

    if (!released && replay.Calculated().flight.release_time.IsDefined()) {
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
  weglide_free.SolveExhaustive();
  charron.SolveExhaustive();

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

  std::cout << "weglide\n";
  std::cout << "# distance\n";
  PrintHelper::print(weglide_free.GetStats().GetResult(0));
  std::cout << "# fai\n";
  PrintHelper::print(weglide_free.GetStats().GetResult(1));
  std::cout << "# out and return\n";
  PrintHelper::print(weglide_free.GetStats().GetResult(2));
  std::cout << "# free\n";
  PrintHelper::print(weglide_free.GetStats().GetResult(3));

  std::cout << "charron\n";
  std::cout << "# free\n";
  PrintHelper::print(charron.GetStats().GetResult(0));

  olc_classic.Reset();
  olc_fai.Reset();
  olc_sprint.Reset();
  olc_league.Reset();
  olc_plus.Reset();
  dmst.Reset();
  olc_netcoupe.Reset();
  weglide_free.Reset();
  charron.Reset();
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

  int result = TestContest(*replay);
  delete replay;
  return result;
}
