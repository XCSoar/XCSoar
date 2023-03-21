// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * Specialisation of Dijkstra for Charron rules.
 * Allows up to 5 legs for distances less than 200km,
 * otherwise 6 legs are allowed. All legs must be longer than 20km.
 *
 * See:
 * https://www.lvzc.be/charronline/2022/overzicht.php
 * https://www.lvzc.be/index.php/secretariaat-2/downloads/charron-line/2013-reglement-charronbeker-2022/file
 */
class Charron : public ContestDijkstra {

  const bool plus_200km;

public:
  /**
   * @param _trace Trace object reference to use for solving
   * @param _plus_200km If true allows 6 legs, but solution distance must be at
   *                    least 200km to be valid.
   */
  explicit Charron(const Trace &_trace, bool _plus_200km) noexcept;


  /**
   * Override Solve to reject solutions under 200km if plus_200km is set.
   */
  SolverResult Solve(bool exhaustive) noexcept override;

protected:
  /* virtual methods from class AbstractContest */
  ContestResult CalculateResult() const noexcept override;
};
