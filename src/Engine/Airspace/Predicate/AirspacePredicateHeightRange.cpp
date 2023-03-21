// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspacePredicateHeightRange.hpp"
#include "../AbstractAirspace.hpp"

bool
AirspacePredicateHeightRange::operator()(const AbstractAirspace& t) const
{
  return t.GetTop().altitude >= h_min &&
    t.GetBase().altitude <= h_max;
}

bool
AirspacePredicateHeightRangeExcludeTwo::operator()(const AbstractAirspace& t) const
{
  return height_range(t) && outside1(t) && outside2(t);
}
