// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DistanceMemento.hpp"

double 
DistanceMemento::Distance(const GeoPoint& _origin,
                          const GeoPoint& _destination) const
{
  if (value < 0 || _origin != origin || _destination != destination) {
    origin = _origin;
    destination = _destination;
    value = origin.Distance(destination);
  }

  return value;
}
