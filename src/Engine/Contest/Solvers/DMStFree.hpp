// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbstractContest.hpp"

/**
 * Composite DMSt distance solver that picks the best result from
 * the three DMSt sub-solvers (quadrilateral, triangle, out-and-return).
 *
 * The highest-scoring sub-result becomes the overall DMSt distance
 * score.  Each sub-solver applies its own bonus before scoring.
 *
 * @see https://github.com/weglide/dmst-wettbewerbsordnung
 */
class DMStFree : public AbstractContest {
  ContestTraceVector solution_quad;
  ContestTraceVector solution_triangle;
  ContestTraceVector solution_or;
  ContestResult result_quad;
  ContestResult result_triangle;
  ContestResult result_or;

public:
  DMStFree() noexcept;

  /**
   * Feed results from DMStQuad, DMStTriangle, and DMStOR.
   * This must be called before this class can do any calculation.
   */
  void Feed(const ContestResult &_result_quad,
            const ContestTraceVector &_solution_quad,
            const ContestResult &_result_triangle,
            const ContestTraceVector &_solution_triangle,
            const ContestResult &_result_or,
            const ContestTraceVector &_solution_or) noexcept {
    result_quad = _result_quad;
    solution_quad = _solution_quad;
    result_triangle = _result_triangle;
    solution_triangle = _solution_triangle;
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

private:
  /**
   * Determine which sub-result has the highest score.
   */
  enum class BestType { QUAD, TRIANGLE, OR };
  [[gnu::pure]]
  BestType GetBestType() const noexcept;
};
