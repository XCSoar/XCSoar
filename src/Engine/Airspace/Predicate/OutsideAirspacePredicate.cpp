// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OutsideAirspacePredicate.hpp"
#include "../AbstractAirspace.hpp"

bool
OutsideAirspacePredicate::operator()(const AbstractAirspace &airspace) const
{
  return !airspace.Inside(location);
}
