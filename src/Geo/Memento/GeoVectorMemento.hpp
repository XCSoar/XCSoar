// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"

/** Memento object to store results of previous GeoVector constructors. */
class GeoVectorMemento 
{
  /** Origin point of saved query */
  mutable GeoPoint origin;

  /** Destination point of previous query */
  mutable GeoPoint destination;

  /** GeoVector saved from previous query */
  mutable GeoVector value;

public:
  /** Constructor, initialises to trigger update on first call. */
  GeoVectorMemento()
    :value(GeoVector::Invalid()) {}

  /**
   * Returns a GeoVector object from the origin to destination, 
   * from previously saved value if input arguments are identical. 
   */
  [[gnu::pure]]
  GeoVector calc(const GeoPoint& _origin, const GeoPoint& _destination) const;
};
