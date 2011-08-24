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

#include "XContestTriangle.hpp"

XContestTriangle::XContestTriangle(const Trace &_trace,
                                   const bool _is_dhv):
  OLCTriangle(_trace),
  is_dhv(_is_dhv) {}

fixed
XContestTriangle::calc_score() const
{
  // DHV-XC: 2.0 or 1.75 points per km for FAI vs non-FAI triangle
  // XContest: 1.4 or 1.2 points per km for FAI vs non-FAI triangle

  const fixed score_factor = is_dhv?
    (is_fai? fixed(0.002): fixed(0.00175))
    :(is_fai? fixed(0.0014): fixed(0.0012));

  return apply_handicap(calc_distance()*score_factor);
}

fixed
XContestTriangle::calc_distance() const
{
  const fixed d_triangle = OLCTriangle::calc_distance();
  if (!positive(d_triangle))
    return fixed_zero;

  // approximation for now: gap is distance from start to finish
  const ScanTaskPoint start(0, 0);
  const ScanTaskPoint end(num_stages-1, n_points-1);

  const fixed d_gap = GetPointFast(start).get_location().distance(GetPointFast(end).get_location());

  // award no points if gap is >20% of triangle

  if (d_gap > fixed(0.2)*d_triangle)
    return fixed_zero;
  else
    return d_triangle - d_gap;
}

bool 
XContestTriangle::solve(bool exhaustive)
{
  if (!ContestDijkstra::solve(exhaustive))
    return false;

  best_d = 0; // reset heuristic

  return true;
}

void
XContestTriangle::swap_mode()
{
  is_fai = !is_fai;
}
