/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

OLCLeague::OLCLeague(const Trace &_trace):
  AbstractContest(_trace, 0)
{
  reset();
}

bool
OLCLeague::save_solution()
{
  if (AbstractContest::save_solution()) {
    std::copy(solution, solution + 5, best_solution);
    return true;
  }
  return false;
}

void
OLCLeague::reset()
{
  AbstractContest::reset();
  std::fill(solution, solution + 5, TracePoint());
  solution_found = false;
  solution_classic.clear();
}

bool
OLCLeague::solve()
{
  TracePointVector trace = trace_master.get_trace_points(2);
  if (trace.size()!=2) {
    return false;
  }

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
OLCLeague::copy_solution(TracePointVector &vec) const
{
  vec.clear();
  vec.reserve(5);
  if (solution_found) {
    for (unsigned i = 0; i < 5; ++i)
      vec.push_back(best_solution[i]);
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
  // @todo: apply handicap *(200/(100+handicap)
  return calc_distance()/fixed(2500);
}


fixed 
OLCLeague::calc_time() const
{
  if (Trace::is_null(solution[4]))
    return fixed_zero;
  else 
    return fixed(solution[4].time - solution[0].time);
}
