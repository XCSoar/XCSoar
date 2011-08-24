/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Trace/Trace.hpp"


AbstractContest::AbstractContest(const Trace &_trace,
                                 const unsigned _finish_alt_diff):
  trace_master(_trace),
  handicap(100),
  finish_alt_diff(_finish_alt_diff)
{
  Reset();
}


void
AbstractContest::Reset()
{
  best_result.reset();
}

bool
AbstractContest::Score(ContestResult &result)
{
  if (positive(CalcTime())) {
    result = best_result;
    return true;
  }
  return false;
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
  const fixed score = CalcScore();
  const bool improved = (score>best_result.score);

  if (improved) {
    best_result.score = score;
    best_result.distance = CalcDistance();
    best_result.time = CalcTime();
    if (positive(best_result.time))
      best_result.speed = best_result.distance / best_result.time;
    else
      best_result.speed = fixed_zero;
    return true;
  }
  return false;
}

bool
AbstractContest::IsFinishAltitudeValid(const TracePoint& start,
                                       const TracePoint& finish) const
{
  return finish.GetIntegerAltitude() + (int)finish_alt_diff >=
    start.GetIntegerAltitude();
}

fixed 
AbstractContest::ApplyHandicap(const fixed& unhandicapped_score,
                                const bool shifted) const
{
  assert(handicap != 0);
  if (handicap == 0) {
    return unhandicapped_score;
  }
  if (shifted) {
    return (200*unhandicapped_score/(100+handicap));
  } else {
    return (100*unhandicapped_score/handicap);
  }
}
