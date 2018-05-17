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

#include "ContestDijkstra.hpp"

AbstractContest::AbstractContest(const unsigned _finish_alt_diff)
  :handicap(100),
   finish_alt_diff(_finish_alt_diff)
{
}

void
AbstractContest::Reset()
{
  best_result.Reset();
}

bool
AbstractContest::UpdateScore()
{
  // for normal contests, nothing needs to be done
  return false;
}

bool
AbstractContest::SaveSolution()
{
  ContestResult result = CalculateResult();
  const bool improved = result.score > best_result.score;

  if (!improved)
    return false;

  best_result = result;
  CopySolution(best_solution);
  return true;
}

bool
AbstractContest::IsFinishAltitudeValid(const TracePoint& start,
                                       const TracePoint& finish) const
{
  return finish.GetIntegerAltitude() + (int)finish_alt_diff >=
    start.GetIntegerAltitude();
}
