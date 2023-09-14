// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  const ContestTraceVector &GetCurrentPath() const noexcept override;

protected:
  /* virtual methods from class AbstractContest */
  ContestResult CalculateResult() const noexcept override;
};
