// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * Specialisation of ContestDijkstra for DMSt out-and-return rules.
 *
 * Section 4.1.5.3: A closed out-and-return flight (start, one
 * turnpoint, finish) receives a +30% bonus.
 *
 * @see https://github.com/weglide/dmst-wettbewerbsordnung
 */
class DMStOR : public ContestDijkstra {
public:
  explicit DMStOR(const Trace &_trace) noexcept;

protected:
  /* virtual methods from AbstractContest */
  ContestResult CalculateResult() const noexcept override;
};
