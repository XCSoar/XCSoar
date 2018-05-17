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
