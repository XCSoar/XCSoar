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

#ifndef OLC_LEAGUE_HPP
#define OLC_LEAGUE_HPP

#include "AbstractContest.hpp"

class Trace;

/**
 * Abstract class for contest searches using dijkstra algorithm
 *
 */
class OLCLeague : public AbstractContest
{
  const Trace &trace;

  ContestTraceVector solution_classic;

  ContestTraceVector solution;

public:
  OLCLeague(const Trace &_trace);

  /**
   * Feed the result from OLCClassic.  This must be called
   * before this class can do any calculation.
   */
  void Feed(const ContestTraceVector &_solution_classic) {
    solution_classic = _solution_classic;
  }

protected:
  virtual ContestResult CalculateResult() const;

public:
  /* virtual methods from class AbstractContest */
  virtual void Reset() override;
  virtual SolverResult Solve(bool exhaustive) override;
  virtual void CopySolution(ContestTraceVector &vec) const override;
};

#endif
