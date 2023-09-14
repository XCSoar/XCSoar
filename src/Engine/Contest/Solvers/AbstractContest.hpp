// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "../ContestResult.hpp"
#include "../ContestTrace.hpp"
#include "Trace/Point.hpp"
#include "PathSolvers/SolverResult.hpp"

#include <cassert>

class TracePoint;

/**
 * Abstract class for contest searches
 *
 */
class AbstractContest {
  unsigned handicap;
  const unsigned finish_alt_diff;
  ContestResult best_result;
  ContestTraceVector best_solution;

public:
  /**
   * Constructor
   *
   * @param _trace Trace object reference to use to retrieve shorter trace for solving
   * @param _handicap Contest handicap factor
   * @param finish_alt_diff Maximum height loss from start to finish (m)
   */
  AbstractContest(const unsigned _finish_alt_diff = 1000) noexcept;

  void SetHandicap(unsigned _handicap) noexcept {
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
  const ContestResult &GetBestResult() const noexcept {
    return best_result;
  }

  const ContestTraceVector &GetBestSolution() const noexcept {
    return best_solution;
  }

protected:
  /**
   * Calculate the result.
   */
  virtual ContestResult CalculateResult() const noexcept = 0;

  /**
   * Return the path for the result returned by CalculateResult().
   */
  [[gnu::pure]]
  virtual const ContestTraceVector &GetCurrentPath() const noexcept = 0;

public:
  /**
   * Reset the optimiser as if never flown
   */
  virtual void Reset() noexcept;

  /**
   * Update the solver.  The solver is incremental, so this method can
   * be safely called every time step.
   *
   * @param exhaustive true to find the final solution, false stops
   * after a number of iterations (incremental search)
   */
  virtual SolverResult Solve(bool exhaustive) noexcept = 0;

protected:
  [[gnu::pure]]
  bool IsFinishAltitudeValid(const TracePoint &start,
                             const TracePoint &finish) const noexcept;

  [[gnu::pure]]
  int GetMaximumStartAltitude(const TracePoint &finish) const noexcept {
    return finish.GetIntegerAltitude() + finish_alt_diff;
  }

  [[gnu::pure]]
  int GetMinimumFinishAltitude(const TracePoint &start) const noexcept {
    return start.GetIntegerAltitude() - finish_alt_diff;
  }

  /**
   * Calculate the score of the current solution (see CalcScore()) and
   * update #best_result (and #best_solution for ContestDijkstra) if
   * it was improved.
   *
   * @return true if #best_result was updated
   */
  bool SaveSolution() noexcept;

  /**
   * Apply handicap.
   *
   * @param unhandicapped_score
   * @param shifted if true, apply (h+100)/200, otherwise h/100
   *
   * @return Handicap adjusted score
   */
  [[gnu::pure]]
  double ApplyHandicap(double unhandicapped_score) const noexcept {
    assert(handicap != 0);

    return 100 * unhandicapped_score / handicap;
  }

  /**
   * Apply "shifted" handicap, i.e. according to OLC league/sprint
   * rules.
   */
  [[gnu::pure]]
  double ApplyShiftedHandicap(const double unhandicapped_score) const noexcept {
    assert(handicap != 0);

    return 400 * unhandicapped_score / (3 * handicap + 100);
  }
};
