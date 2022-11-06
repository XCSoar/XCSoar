/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

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
