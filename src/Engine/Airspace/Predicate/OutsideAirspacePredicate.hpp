// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

class AbstractAirspace;

/**
 * Matches all airspaces which do not include the given location.
 */
class OutsideAirspacePredicate {
  const AGeoPoint location;

public:
  constexpr OutsideAirspacePredicate(const AGeoPoint &_location)
    :location(_location) {}

  [[gnu::pure]]
  bool operator()(const AbstractAirspace &airspace) const;
};
