// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

  [[gnu::pure]]
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
