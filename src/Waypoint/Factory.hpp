// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Waypoint.hpp"

class RasterTerrain;

/**
 * This class can create new Waypoint instances and can help fill out
 * some attributes.  It is meant to encapsulate the initialisation of
 * attributes that are not specific to the #WaypointReaderBase
 * implementation.
 */
class WaypointFactory {
  WaypointOrigin origin;
  const RasterTerrain *terrain;

public:
  explicit WaypointFactory(WaypointOrigin _origin,
                           const RasterTerrain *_terrain=nullptr) noexcept
    :origin(_origin), terrain(_terrain) {}

  [[gnu::pure]]
  Waypoint Create(const GeoPoint &location) const noexcept {
    Waypoint w(location);
    w.origin = origin;
    w.original_id = 0;
    return w;
  }

  /**
   * We don't know the elevation of this waypoint, and this method
   * shall find a viable fallback (e.g. by looking up the terrain
   * elevation at the given location).
   *
   * @return true if a fallback was found and Waypoint::elevation was
   * set, false if no fallback was found
   */
  bool FallbackElevation(Waypoint &waypoint) const noexcept;
};
