/* Copyright_License {

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

#include "OLCPlus.hpp"

OLCPlus::OLCPlus()
  :AbstractContest(0)
{
}

void
OLCPlus::Reset()
{
  AbstractContest::Reset();
  solution_classic.clear();
  solution_fai.clear();
  result_classic.Reset();
  result_fai.Reset();
}

SolverResult
OLCPlus::Solve(bool exhaustive)
{
  return SaveSolution()
    ? SolverResult::VALID
    : SolverResult::FAILED;
}

void
OLCPlus::CopySolution(ContestTraceVector &vec) const
{
  vec = solution_classic;
}

ContestResult
OLCPlus::CalculateResult() const
{
  ContestResult result = result_classic;
  result.score = ApplyHandicap((result_classic.distance +
                                0.3 * result_fai.distance) / 1000);
  return result;
}
