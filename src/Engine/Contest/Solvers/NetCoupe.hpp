// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * Specialisation of Contest Dijkstra for FFVV NetCoupe rules
 */
class NetCoupe : public ContestDijkstra {
public:
  explicit NetCoupe(const Trace &_trace) noexcept;

protected:
  /* virtual methods from class AbstractContest */
  ContestResult CalculateResult() const noexcept override;
};
