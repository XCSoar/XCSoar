/* Copyright_License {

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
#include "OLCLeague.hpp"
#include "Trace/Trace.hpp"
#include <assert.h>

OLCLeague::OLCLeague(const Trace &_trace,
                     const unsigned &_handicap):
  AbstractContest(_trace, _handicap, 0)
{
  reset();
}

bool
OLCLeague::save_solution()
{
  if (AbstractContest::save_solution()) {
    best_solution = solution;
    return true;
  }
  return false;
}

void
OLCLeague::reset()
{
  AbstractContest::reset();
  solution_found = false;
  solution.clear();
  solution_classic.clear();
  for (unsigned i=0; i<5; ++i) {
    solution.append().Clear();
  }
}

bool
OLCLeague::solve(bool exhaustive)
{
  if (trace_master.size() < 2)
    return false;

  const TracePoint trace[2] = {
    trace_master.front(),
    trace_master.back(),
  };

  if (!finish_altitude_valid(trace[0], trace[1])) {
    return false;
  }

  // solution found, so set start/finish points
  solution[0] = trace[0];
  solution[4] = trace[1];

  // scan through classic solution to find points there to add

  unsigned index_fill = 1;

  for (unsigned index_classic = 1; index_classic+1 < solution_classic.size(); ++index_classic) {
    if ((solution_classic[index_classic].time > solution[index_fill-1].time)
        &&(solution_classic[index_classic].time < trace[1].time)) {

      solution[index_fill] = solution_classic[index_classic];
      index_fill++;
      if (index_fill==4) {
        break;
      }
    }
  }

  // if insufficient points found, add repeats of previous points

  for (; index_fill<4; ++index_fill) {
    solution[index_fill] = solution[index_fill-1];
  }

  solution_found = true;

  return true;
}


bool 
OLCLeague::score(ContestResult &result)
{
  if (solution_found) {
    save_solution();
    return AbstractContest::score(result);
  }
  return false;
}


void 
OLCLeague::copy_solution(ContestTraceVector &vec) const
{
  vec.clear();
  if (solution_found) {
    vec = best_solution;
  }
}


fixed 
OLCLeague::calc_distance() const 
{
  fixed dist = fixed_zero;
  for (unsigned i = 0; i < 4; ++i)
    dist += solution[i].distance(solution[i + 1].get_location());

  return dist;
}


fixed 
OLCLeague::calc_score() const
{
  return apply_handicap(calc_distance()/fixed(2500), true);
}


fixed 
OLCLeague::calc_time() const
{
  if (!solution[4].Defined())
    return fixed_zero;
  else 
    return fixed(solution[4].time - solution[0].time);
}
