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

#include "ScanTaskPoint.hpp"
#include "Math/fixed.hpp"
#include "Task/TaskStats/ContestResult.hpp"

class Trace;
class TracePoint;

/**
 * Abstract class for contest searches
 *
 */
class AbstractContest
{
public:
  /**
   * Constructor
   *
   * @param _trace Trace object reference to use to retrieve shorter trace for solving
   * @param _handicap Contest handicap factor
   * @param finish_alt_diff Maximum height loss from start to finish (m)
   */
  AbstractContest(const Trace &_trace,
                  const unsigned &_handicap,
                  const unsigned finish_alt_diff = 1000);

  /**
   * Calculate the scored values of the Contest path
   *
   * @param result The ContestResult reference
   * in which the solution will be written
   *
   * @return True is solution was found, False otherwise
   */
  virtual bool score(ContestResult &result);

  /**
   * Copy the best Contest path solution
   *
   * @param vec output vector
   */
  virtual void copy_solution(ContestTraceVector &vec) const = 0;

protected:
  /**
   * Calculate distance of best path
   *
   * @return Distance (m)
   */
  virtual fixed calc_distance() const = 0;

  /**
   * Calculate score of best path
   * This is specialised by each contest type, defaults to
   * speed in kph measured by weighted distance divided by time
   * 
   * @return Score (pts)
   */
  virtual fixed calc_score() const = 0;

  /**
   * Calculate elapsed time of best path
   *
   * @return Distance (m)
   */
  virtual fixed calc_time() const = 0;

public:
  /**
   * Reset the optimiser as if never flown
   */
  virtual void reset();

  /**
   * Update the solver.  The solver is incremental, so this method can
   * be safely called every time step.
   *
   * @param exhaustive true to find the final solution, false stops
   * after a number of iterations (incremental search)
   * @return True if solver completed in this call
   */
  virtual bool solve(bool exhaustive) = 0;

protected:

  /**
   * Perform check on whether score needs to be
   * updated (even if score isn't improved, due to
   * new conditions occuring, e.g. closure of path)
   *
   * @return true if score is updated
   */
  virtual bool update_score();

  bool finish_altitude_valid(const TracePoint& start,
                             const TracePoint& finish) const;

  const Trace &trace_master;
  virtual bool save_solution();

  /**
   * Apply handicap.
   *
   * @param unhandicapped_score
   * @param shifted if true, apply (h+100)/200, otherwise h/100
   *
   * @return Handicap adjusted score
   */
  fixed apply_handicap(const fixed& unhandicapped_score, const bool shifted=false) const;

private:
  const unsigned &contest_handicap;
  const unsigned m_finish_alt_diff;
  ContestResult best_result;
};

#endif
