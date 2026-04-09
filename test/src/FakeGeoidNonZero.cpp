// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo/Geoid.hpp"
#include "Geo/GeoPoint.hpp"

double
EGM96::LookupSeparation(const GeoPoint &)
{
  /* Non-zero so tests can verify datum conversion logic. */
  return 30;
}

