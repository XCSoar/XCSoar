// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Contest/Solvers/AbstractContest.hpp"
#include "Engine/Contest/ContestResult.hpp"
#include "Engine/Contest/ContestTrace.hpp"
#include "PathSolvers/SolverResult.hpp"
#include "TestUtil.hpp"
#include "util/PrintException.hxx"

#include <cassert>

static void
TestSetHandicap()
{
  // Create a mock contest class to test SetHandicap
  class TestContest : public AbstractContest {
  public:
    TestContest() : AbstractContest(1000) {}
    ContestResult CalculateResult() const noexcept override {
      return ContestResult();
    }
    const ContestTraceVector &GetCurrentPath() const noexcept override {
      static ContestTraceVector empty;
      return empty;
    }
    SolverResult Solve(bool) noexcept override {
      return SolverResult::FAILED;
    }
    
    // Expose protected methods for testing
    using AbstractContest::ApplyHandicap;
    using AbstractContest::ApplyShiftedHandicap;
  };

  TestContest contest;

  // Test 1: Setting handicap to 0 should default to 100
  contest.SetHandicap(0);
  double result1 = contest.ApplyHandicap(1000.0);
  ok1(equals(result1, 1000.0)); // 100 * 1000 / 100 = 1000

  // Test 2: Setting handicap to valid value should work
  contest.SetHandicap(120);
  double result2 = contest.ApplyHandicap(1000.0);
  ok1(equals(result2, 833.33333333333337)); // 100 * 1000 / 120 ≈ 833.33

  // Test 3: ApplyShiftedHandicap with 0 should default to 100
  contest.SetHandicap(0);
  double shifted = contest.ApplyShiftedHandicap(1000.0);
  ok1(equals(shifted, 1000.0)); // 400 * 1000 / (3 * 100 + 100) = 1000

  // Test 4: ApplyShiftedHandicap with valid value
  contest.SetHandicap(120);
  shifted = contest.ApplyShiftedHandicap(1000.0);
  ok1(equals(shifted, 869.56521739130438)); // 400 * 1000 / (3 * 120 + 100) ≈ 869.57
}

int main()
try {
  plan_tests(4);

  TestSetHandicap();

  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
