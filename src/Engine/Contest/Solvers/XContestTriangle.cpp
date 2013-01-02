/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
                                   bool predict, bool _is_dhv)
  :OLCTriangle(_trace, true, predict),
   is_dhv(_is_dhv) {}

ContestResult
XContestTriangle::CalculateResult(const ContestTraceVector &solution) const
{
  ContestResult result = OLCTriangle::CalculateResult(solution);

  if (positive(result.distance)) {
    // approximation for now: gap is distance from start to finish
    const fixed d_gap = GetPoint(0).GetLocation()
      .Distance(GetPoint(n_points - 1).GetLocation());

    // award no points if gap is >20% of triangle

    if (d_gap > fixed(0.2) * result.distance)
      result.distance = fixed(0);
    else
      result.distance -= d_gap;
  } else
    result.distance = fixed(0);

  // DHV-XC: 2.0 or 1.75 points per km for FAI vs non-FAI triangle
  // XContest: 1.4 or 1.2 points per km for FAI vs non-FAI triangle

  const fixed score_factor = is_dhv?
    (is_fai? fixed(0.002): fixed(0.00175))
    :(is_fai? fixed(0.0014): fixed(0.0012));

  result.score = ApplyHandicap(result.distance * score_factor);
  return result;
}

SolverResult
XContestTriangle::Solve(bool exhaustive)
{
  SolverResult result = ContestDijkstra::Solve(exhaustive);
  if (result != SolverResult::FAILED)
    best_d = 0; // reset heuristic

  return result;
}
