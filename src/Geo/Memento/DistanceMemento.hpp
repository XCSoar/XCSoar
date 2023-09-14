// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

/** Memento object to store results of previous distance calculations. */
class DistanceMemento
{
  /** Origin point of saved query */
  mutable GeoPoint origin;

  /** Destination point of previous query */
  mutable GeoPoint destination;

  /** Distance in meters saved from previous query */
  mutable double value = -1.0;

public:
  /**
   * Returns the distance from the origin to destination in meters, 
   * from previously saved value if input arguments are identical. 
   */
  [[gnu::pure]]
  double Distance(const GeoPoint& _origin, const GeoPoint& _destination) const;
};
