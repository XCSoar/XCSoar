// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "util/TrivialArray.hxx"

#include <vector>

/**
 * A Route is a vector of AGeoPoints.
 */
typedef std::vector<AGeoPoint> Route;

/**
 * A variant of Route based on StaticArray.
 */
struct StaticRoute : public TrivialArray<Route::value_type, 64u> {
  /**
   * Copy a Route to a StaticRoute, clipping items that don't fit.
   */
  StaticRoute &operator=(const Route &src) {
    clear();
    for (auto i = src.begin(), end = src.end(); i != end && !full(); ++i)
      push_back(*i);
    return *this;
  }
};
