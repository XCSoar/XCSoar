// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbstractContest.hpp"

/**
 * Abstract class for contest searches using dijkstra algorithm
 *
 */
class OLCPlus : public AbstractContest {
  ContestTraceVector solution_classic;
  ContestTraceVector solution_fai;
  ContestResult result_classic;
  ContestResult result_fai;

public:
  OLCPlus() noexcept;

  /**
   * Feed results from OLCClassic and OLCFAI.  This must be called
   * before this class can do any calculation.
   */
  void Feed(const ContestResult &_result_classic,
            const ContestTraceVector &_solution_classic,
            const ContestResult &_result_fai,
            const ContestTraceVector &_solution_fai) noexcept {
    result_classic = _result_classic;
    solution_classic = _solution_classic;
    result_fai = _result_fai;
    solution_fai = _solution_fai;
  }

public:
  /* virtual methods from class AbstractContest */
  void Reset() noexcept override;
  SolverResult Solve(bool exhaustive) noexcept override;
  const ContestTraceVector &GetCurrentPath() const noexcept override;

protected:
  /* virtual methods from class AbstractContest */
  ContestResult CalculateResult() const noexcept override;
};
