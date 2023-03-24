// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * Specialisation of ContestDijkstra for XContest and DHV-XC free flight rules
 */
class XContestFree : public ContestDijkstra {
  const bool is_dhv;

public:
  XContestFree(const Trace &_trace,
               const bool _is_dhv=false) noexcept;

protected:
  /* virtual methods from AbstractContest */
  ContestResult CalculateResult() const noexcept override;
};
