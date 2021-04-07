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

#ifndef WEGLIDE_FREE_HPP
#define WEGLIDE_FREE_HPP

#include "AbstractContest.hpp"

/**
 * Abstract class for contest searches using dijkstra algorithm
 *
 */
class WeglideFree : public AbstractContest {
  ContestTraceVector solution_distance;
  ContestTraceVector solution_fai;
  ContestTraceVector solution_or;
  ContestResult result_distance;
  ContestResult result_fai;
  ContestResult result_or;

public:
  WeglideFree() noexcept;

  /**
   * Feed results from WeglideDistance, WeglideFAI and WeglideOR. This must be called
   * before this class can do any calculation.
   */
  void Feed(const ContestResult &_result_distance,
            const ContestTraceVector &_solution_distance,
            const ContestResult &_result_fai,
            const ContestTraceVector &_solution_fai,
            const ContestResult &_result_or,
            const ContestTraceVector &_solution_or) noexcept {
    result_distance = _result_distance;
    solution_distance = _solution_distance;
    result_fai = _result_fai;
    solution_fai = _solution_fai;
    result_or = _result_or;
    solution_or = _solution_or;
  }

public:
  /* virtual methods from class AbstractContest */
  void Reset() noexcept override;
  SolverResult Solve(bool exhaustive) noexcept override;
  void CopySolution(ContestTraceVector &vec) const noexcept override;

protected:
  /* virtual methods from class AbstractContest */
  ContestResult CalculateResult() const noexcept override;
};

#endif
