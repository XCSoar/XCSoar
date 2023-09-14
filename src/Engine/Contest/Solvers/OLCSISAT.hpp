// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * Specialisation of Dijkstra for SIS-AT rules
 */
class OLCSISAT : public ContestDijkstra {
public:
  explicit OLCSISAT(const Trace &_trace) noexcept;

protected:
  /* virtual methods from class ContestDijkstra */
  ContestResult CalculateResult(const ContestTraceVector &solution) const noexcept override;
};
