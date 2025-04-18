// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GeoVectorMemento.hpp"

GeoVector 
GeoVectorMemento::calc(const GeoPoint& _origin,
                       const GeoPoint& _destination) const
{
  if (!value.IsValid() ||
      _origin != origin ||
      _destination != destination) {
    origin = _origin;
    destination = _destination;
    value = GeoVector(origin, destination);
  }

  return value;
}
