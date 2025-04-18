// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TriangleContest.hpp"

/**
 * Specialisation of ContestDijkstra for XContest and DHV-XC triangle rules.
 *
 * This solver alternates between searching for FAI and non-FAI triangles
 */
class XContestTriangle : public TriangleContest {
  const bool is_dhv;

public:
  XContestTriangle(const Trace &_trace, bool predict, bool _is_dhv) noexcept;

protected:
  /* virtual methods from AbstractContest */
  SolverResult Solve(bool exhaustive) noexcept override;

  /* virtual methods from TriangleContest */
  ContestResult CalculateResult() const noexcept override;
};
