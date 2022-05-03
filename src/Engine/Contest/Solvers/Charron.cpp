/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Charron.hpp"


Charron::Charron(const Trace &_trace, bool _plus_200km) noexcept
    : ContestDijkstra(_trace, true, _plus_200km ? 6 : 5, 1000, 20'000),
      plus_200km(_plus_200km)
{
}

SolverResult
Charron::Solve(bool exhaustive) noexcept
{
  const auto result = ContestDijkstra::Solve(exhaustive);
  if (result != SolverResult::VALID) {
    return result;
  }

  const auto &solution = GetBestSolution();

  auto distance = 0.0;
  GeoPoint previous = solution[0].GetLocation();
  for (unsigned i = 1; i < num_stages; ++i) {
    const GeoPoint &current = solution[i].GetLocation();
    distance += current.Distance(previous);
    previous = current;
  }

  // Reject the solution if the distance was less than 200km.
  // Also reset the best solution set by ContestDijkstra::Solve.
  if (plus_200km && distance < 200'000) {
    AbstractContest::Reset();
    return SolverResult::FAILED;
  }

  return result;
}

ContestResult
Charron::CalculateResult() const noexcept
{
  ContestResult result = ContestDijkstra::CalculateResult();

  // 4 points per km.
  const auto score_factor = 4 / 1000.0;
  result.score = ApplyHandicap(result.distance * score_factor);

  return result;

}
