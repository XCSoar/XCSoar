// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/**
 * This file handles the geoid separation
 * @file Geoid.cpp
 * @see http://en.wikipedia.org/wiki/EGM96
 */

#include "Geoid.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Util.hpp"

#include <cstdint>

static constexpr int EGM96SIZE = 16200;

extern "C" const uint8_t egm96s_dem[];

double
EGM96::LookupSeparation(const GeoPoint &pt)
{
  int ilat, ilon;
  ilat = iround((Angle::QuarterCircle() - pt.latitude).Half().Degrees());
  ilon = iround(pt.longitude.AsBearing().Half().Degrees());

  int offset = ilat * 180 + ilon;
  if (offset >= EGM96SIZE)
    return 0;
  if (offset < 0)
    return 0;

  return (int)egm96s_dem[offset] - 127;
}
