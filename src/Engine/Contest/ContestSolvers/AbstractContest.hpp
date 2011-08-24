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

#ifndef ABSTRACT_CONTEST_HPP
#define ABSTRACT_CONTEST_HPP

#include "Task/Tasks/PathSolvers/ScanTaskPoint.hpp"
#include "Math/fixed.hpp"
#include "../ContestResult.hpp"

class Trace;
class TracePoint;

/**
 * Abstract class for contest searches
 *
 */
class AbstractContest
{
protected:
  const Trace &trace_master;

private:
  unsigned handicap;
  const unsigned finish_alt_diff;
  ContestResult best_result;

public:
  /**
   * Constructor
   *
   * @param _trace Trace object reference to use to retrieve shorter trace for solving
   * @param _handicap Contest handicap factor
   * @param finish_alt_diff Maximum height loss from start to finish (m)
   */
  AbstractContest(const Trace &_trace,
                  const unsigned _finish_alt_diff = 1000);

  void SetHandicap(unsigned _handicap) {
    handicap = _handicap;
  }

  /**
   * Calculate the scored values of the Contest path
   *
   * @param result The ContestResult reference
   * in which the solution will be written
   *
   * @return True is solution was found, False otherwise
   */
  virtual bool Score(ContestResult &result);

  /**
   * Copy the best Contest path solution
   *
   * @param vec output vector
   */
  virtual void CopySolution(ContestTraceVector &vec) const = 0;

protected:
  /**
   * Calculate distance of best path
   *
   * @return Distance (m)
   */
  virtual fixed CalcDistance() const = 0;

  /**
   * Calculate score of best path
   * This is specialised by each contest type, defaults to
   * speed in kph measured by weighted distance divided by time
   * 
   * @return Score (pts)
   */
  virtual fixed CalcScore() const = 0;

  /**
   * Calculate elapsed time of best path
   *
   * @return Distance (m)
   */
  virtual fixed CalcTime() const = 0;

public:
  /**
   * Reset the optimiser as if never flown
   */
  virtual void Reset();

  /**
   * Update the solver.  The solver is incremental, so this method can
   * be safely called every time step.
   *
   * @param exhaustive true to find the final solution, false stops
   * after a number of iterations (incremental search)
   * @return True if solver completed in this call
   */
  virtual bool Solve(bool exhaustive) = 0;

protected:

  /**Applied naming convention
   * Perform check on whether score needs to be
   * updated (even if score isn't improved, due to
   * new conditions occuring, e.g. closure of path)
   *
   * @return true if score is updated
   */
  virtual bool UpdateScore();

  bool IsFinishAltitudeValid(const TracePoint& start,
                             const TracePoint& finish) const;

  virtual bool SaveSolution();

  /**
   * Apply handicap.
   *
   * @param unhandicapped_score
   * @param shifted if true, apply (h+100)/200, otherwise h/100
   *
   * @return Handicap adjusted score
   */
  fixed ApplyHandicap(const fixed& unhandicapped_score, const bool shifted=false) const;
};

#endif
