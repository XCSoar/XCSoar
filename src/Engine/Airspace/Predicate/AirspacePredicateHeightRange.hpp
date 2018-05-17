/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef AIRSPACE_PREDICATE_HEIGHT_RANGE_HPP
#define AIRSPACE_PREDICATE_HEIGHT_RANGE_HPP

#include "OutsideAirspacePredicate.hpp"

class AbstractAirspace;

/**
 * Convenience predicate for height within a specified range
 */
class AirspacePredicateHeightRange
{
  const double h_min;
  const double h_max;

public:
  /**
   * Constructor
   *
   * @param _h_min Lower bound on airspace (m)
   * @param _h_max Upper bound on airspace (m)
   *
   * @return Initialised object
   */
  constexpr
  AirspacePredicateHeightRange(const double _h_min,
                               const double _h_max)
    :h_min(_h_min), h_max(_h_max) {}

  gcc_pure
  bool operator()(const AbstractAirspace &t) const;
};

/**
 * Convenience predicate for height within a specified range, excluding
 * airspaces enclosing two points
 */
class AirspacePredicateHeightRangeExcludeTwo
{
  const AirspacePredicateHeightRange height_range;
  const OutsideAirspacePredicate outside1, outside2;

public:
  /**
   * Constructor
   *
   * @param _h_min Lower bound on airspace (m)
   * @param _h_max Upper bound on airspace (m)
   *
   * @return Initialised object
   */
  AirspacePredicateHeightRangeExcludeTwo(const double _h_min,
                                         const double _h_max,
                                         const AGeoPoint& _p1,
                                         const AGeoPoint& _p2)
    :height_range(_h_min, _h_max),
     outside1(_p1), outside2(_p2) {}

  bool operator()(const AbstractAirspace &t) const;
};

#endif
