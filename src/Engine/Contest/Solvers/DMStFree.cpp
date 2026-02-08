// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DMStFree.hpp"

DMStFree::DMStFree() noexcept
  :AbstractContest(0)
{
}

void
DMStFree::Reset() noexcept
{
  AbstractContest::Reset();
  solution_quad.clear();
  solution_triangle.clear();
  solution_or.clear();
  result_quad.Reset();
  result_triangle.Reset();
  result_or.Reset();
}

SolverResult
DMStFree::Solve([[maybe_unused]] bool exhaustive) noexcept
{
  return SaveSolution()
    ? SolverResult::VALID
    : SolverResult::FAILED;
}

DMStFree::BestType
DMStFree::GetBestType() const noexcept
{
  if (result_triangle.score >= result_quad.score &&
      result_triangle.score >= result_or.score)
    return BestType::TRIANGLE;

  if (result_or.score >= result_quad.score)
    return BestType::OR;

  return BestType::QUAD;
}

const ContestTraceVector &
DMStFree::GetCurrentPath() const noexcept
{
  switch (GetBestType()) {
  case BestType::TRIANGLE:
    return solution_triangle;
  case BestType::OR:
    return solution_or;
  case BestType::QUAD:
    break;
  }

  return solution_quad;
}

ContestResult
DMStFree::CalculateResult() const noexcept
{
  switch (GetBestType()) {
  case BestType::TRIANGLE:
    return result_triangle;
  case BestType::OR:
    return result_or;
  case BestType::QUAD:
    break;
  }

  return result_quad;
}
