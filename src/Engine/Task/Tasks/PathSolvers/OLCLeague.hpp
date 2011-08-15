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

#ifndef OLC_LEAGUE_HPP
#define OLC_LEAGUE_HPP

#include "AbstractContest.hpp"

/**
 * Abstract class for contest searches using dijkstra algorithm
 *
 */
class OLCLeague:
  public AbstractContest
{
  ContestTraceVector solution_classic;

  ContestTraceVector best_solution;
  ContestTraceVector solution;
  bool solution_found;

public:
  OLCLeague(const Trace &_trace,
    const unsigned& _handicap);

  bool score(ContestResult &result);

  virtual void copy_solution(ContestTraceVector &vec) const;

protected:
  virtual fixed calc_distance() const;
  virtual fixed calc_score() const;
  virtual fixed calc_time() const;

public:
  /**
   * Reset the optimiser as if never flown
   */
  virtual void reset();

  /**
   * Update the solver.  The solver is incremental, so this method can
   * be safely called every time step.
   *
   * @return True if solver completed in this call
   */
  virtual bool solve(bool exhaustive);

  ContestTraceVector& get_solution_classic() {
    return solution_classic;
  }

protected:
  virtual bool save_solution();
};

#endif
