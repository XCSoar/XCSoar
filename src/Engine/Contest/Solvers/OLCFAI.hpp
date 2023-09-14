// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TriangleContest.hpp"

/**
 * Specialisation of OLC Triangle with OLC FAI (triangle) rules
 */
class OLCFAI : public TriangleContest {
public:
  OLCFAI(const Trace &_trace, bool predict) noexcept;

protected:
  /* virtual methods from class TriangleContest */
  ContestResult CalculateResult() const noexcept override;
};
