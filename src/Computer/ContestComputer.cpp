// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ContestComputer.hpp"
#include "Engine/Contest/Settings.hpp"

ContestComputer::ContestComputer(const Trace &trace_full,
                                 const Trace &trace_triangle,
                                 const Trace &trace_sprint)
  :contest_manager(Contest::OLC_SPRINT, trace_full, trace_triangle, trace_sprint, true)
{
  contest_manager.SetIncremental(true);
}

void
ContestComputer::Solve(const ContestSettings &settings,
                       ContestStatistics &contest_stats)
{
  if (!settings.enable)
    return;

  contest_manager.SetHandicap(settings.handicap);
  contest_manager.SetContest(settings.contest);

  contest_manager.UpdateIdle();

  contest_stats = contest_manager.GetStats();
}

bool
ContestComputer::SolveExhaustive(const ContestSettings &settings,
                                 ContestStatistics &contest_stats)
{
  if (!settings.enable)
    return false;

  contest_manager.SetHandicap(settings.handicap);
  contest_manager.SetContest(settings.contest);

  bool result = contest_manager.SolveExhaustive();

  contest_stats = contest_manager.GetStats();

  return result;
}
