// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  explicit OLCLeague(const Trace &_trace) noexcept;

  /**
   * Feed the result from OLCClassic.  This must be called
   * before this class can do any calculation.
   */
  void Feed(const ContestTraceVector &_solution_classic) noexcept {
    solution_classic = _solution_classic;
  }

protected:
  ContestResult CalculateResult() const noexcept override;

public:
  /* virtual methods from class AbstractContest */
  void Reset() noexcept override;
  SolverResult Solve(bool exhaustive) noexcept override;
  const ContestTraceVector &GetCurrentPath() const noexcept override;
};
