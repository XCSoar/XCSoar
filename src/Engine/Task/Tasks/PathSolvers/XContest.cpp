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
#include "XContest.hpp"
#include <assert.h>

XContest::XContest(const Trace &_trace,
                   const unsigned &_handicap,
                   const bool is_dhv):
  AbstractContest(_trace, _handicap, 1000),
  solver_free(_trace, _handicap, is_dhv),
  solver_triangle(_trace, _handicap, is_dhv)
{
  reset();
}

void
XContest::reset()
{
  AbstractContest::reset();
  solver_free.reset();
  solver_triangle.reset();
  mode_free = true;
  x_best_solution.clear();
}

bool
XContest::solve()
{
  if (mode_free) {

    if (solver_free.solve()) {
      if (save_solution())
        solver_free.copy_solution(x_best_solution);
      mode_free = false;
      return true;
    } else 
      return false;

  } else {

    if (solver_triangle.solve()) {
      if (save_solution())
        solver_free.copy_solution(x_best_solution);
      mode_free = true;
      solver_triangle.swap_mode();
      return true;
    } else
      return false;
  }
}


void 
XContest::copy_solution(TracePointVector &vec) const
{
  vec.clear();
  if (x_best_solution.empty())
    return;

  vec.reserve(x_best_solution.size());
  for (unsigned i = 0; i < x_best_solution.size(); ++i)
    vec.push_back(x_best_solution[i]);
}


fixed 
XContest::calc_distance() const 
{
  if (mode_free) 
    return solver_free.calc_distance();
  else 
    return solver_triangle.calc_distance();
}


fixed 
XContest::calc_score() const
{
  if (mode_free) 
    return solver_free.calc_score();
  else 
    return solver_triangle.calc_score();
}


fixed 
XContest::calc_time() const
{
  if (mode_free) 
    return solver_free.calc_time();
  else 
    return solver_triangle.calc_time();
}
