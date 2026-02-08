// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * A quadrilateral task for the DMSt (Deutsche Meisterschaft im
 * Streckensegelflug).
 *
 * Section 4.1: Distance scoring with up to 3 turnpoints and a
 * maximum 1000m altitude difference between start and finish.
 * No bonus applied (bonuses require triangle/OR shape detection
 * or declared task information).
 *
 * @see https://github.com/weglide/dmst-wettbewerbsordnung
 */
class DMStQuad : public ContestDijkstra {
public:
  explicit DMStQuad(const Trace &_trace) noexcept;

protected:
  /* virtual methods from AbstractContest */
  ContestResult CalculateResult() const noexcept override;
};
