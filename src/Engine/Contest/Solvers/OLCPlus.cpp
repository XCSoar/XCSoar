// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OLCPlus.hpp"

OLCPlus::OLCPlus() noexcept
  :AbstractContest(0)
{
}

void
OLCPlus::Reset() noexcept
{
  AbstractContest::Reset();
  solution_classic.clear();
  solution_fai.clear();
  result_classic.Reset();
  result_fai.Reset();
}

SolverResult
OLCPlus::Solve([[maybe_unused]] bool exhaustive) noexcept
{
  return SaveSolution()
    ? SolverResult::VALID
    : SolverResult::FAILED;
}

const ContestTraceVector &
OLCPlus::GetCurrentPath() const noexcept
{
  return solution_classic;
}

ContestResult
OLCPlus::CalculateResult() const noexcept
{
  ContestResult result = result_classic;
  result.score = ApplyHandicap((result_classic.distance +
                                0.3 * result_fai.distance) / 1000);
  return result;
}
