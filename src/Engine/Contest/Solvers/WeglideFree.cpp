/* Copyright_License {

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

#include "WeglideFree.hpp"

WeglideFree::WeglideFree() noexcept
  :AbstractContest(0)
{
}

void
WeglideFree::Reset() noexcept
{
  AbstractContest::Reset();
  solution_distance.clear();
  solution_or.clear();
  solution_fai.clear();
  result_distance.Reset();
  result_or.Reset();
  result_fai.Reset();
}

SolverResult
WeglideFree::Solve(bool exhaustive) noexcept
{
  return SaveSolution()
    ? SolverResult::VALID
    : SolverResult::FAILED;
}

void
WeglideFree::CopySolution(ContestTraceVector &vec) const noexcept
{
  vec = solution_distance;
}

ContestResult
WeglideFree::CalculateResult() const noexcept
{
  ContestResult result = result_distance;

  auto area_score = 
    (result_or.distance * 0.2 > result_fai.distance * 0.3)
    ? (result_or.distance * 0.2)
    : (result_fai.distance * 0.3);

  result.score = ApplyHandicap((result_distance.distance + area_score) 
                                / 1000);

  return result;
}
