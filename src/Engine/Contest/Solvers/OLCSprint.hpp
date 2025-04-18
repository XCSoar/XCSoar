// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * Specialisation of Dijkstra for OLC Sprint (also known as OLC League) rules
 */
class OLCSprint : public ContestDijkstra {
public:
  /**
   * Constructor
   */
  explicit OLCSprint(const Trace &_trace) noexcept;

private:
  [[gnu::pure]]
  unsigned FindStart() const noexcept;

protected:
  /* virtual methods from AbstractContest */
  ContestResult CalculateResult() const noexcept override;

  /* virtual methods from NavDijkstra */
  void AddEdges(ScanTaskPoint origin) noexcept override;

  /* virtual methods from ContestDijkstra */
  void UpdateTrace(bool force) noexcept override;
  void AddStartEdges() noexcept override;
};
