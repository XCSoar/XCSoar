// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;

namespace EGM96
{
  /**
   * Returns the geoid separation between the EGS96
   * and the WGS84 at the given latitude and longitude
   * @param lat Latitude
   * @param lon Longitude
   * @return The geoid separation
   */
  [[gnu::pure]]
  double LookupSeparation(const GeoPoint &pt);
}
