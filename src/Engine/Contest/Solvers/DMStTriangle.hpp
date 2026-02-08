// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TriangleContest.hpp"

/**
 * Specialisation of TriangleContest for DMSt triangle rules.
 *
 * Section 4.1.5.1: A closed triangle flight receives a +40% bonus
 * if the triangle meets the FAI/OLC leg ratio constraints:
 * - Small (<500km): shortest leg >= 28% of total
 * - Large (>=500km): shortest >= 25%, longest <= 45%
 *
 * @see https://github.com/weglide/dmst-wettbewerbsordnung
 */
class DMStTriangle : public TriangleContest {
public:
  DMStTriangle(const Trace &_trace, bool predict) noexcept;

protected:
  /* virtual methods from class TriangleContest */
  ContestResult CalculateResult() const noexcept override;
};
