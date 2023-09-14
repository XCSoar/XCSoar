// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TriangleContest.hpp"

/**
 * Specialisation of Weglide Triangle with Weglide FAI (triangle) rules
 */
class WeglideFAI : public TriangleContest {
public:
  WeglideFAI(const Trace &_trace, bool predict) noexcept;

protected:
  /* virtual methods from class TriangleContest */
  ContestResult CalculateResult() const noexcept override;
};
