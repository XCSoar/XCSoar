// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Terrain/CogTileId.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"

namespace Event {

/**
 * Posted when a 1-degree COG terrain tile header has been loaded
 * and is ready for height queries.
 */
struct TerrainTileLoadedEvent {
  CogTileId tile_id;
  GeoBounds bounds;
};

/**
 * Posted when an internal tile (512x512 block within a COG file)
 * has been fetched and cached.
 */
struct TerrainInternalTileLoadedEvent {
  CogTileId tile_id;
  unsigned level;
  unsigned tx, ty;
};

/**
 * Posted when terrain loading fails for a tile.
 */
struct TerrainTileFailedEvent {
  CogTileId tile_id;
  const char *reason;
};

/**
 * Posted when all terrain in the current view is loaded.
 */
struct TerrainReadyEvent {};

/**
 * Posted when the map view changes, requesting terrain for the new area.
 */
struct ViewChangedEvent {
  GeoPoint center;
  double radius_meters;
  unsigned quantisation;
};

/**
 * Request to pre-cache terrain tiles for a specific area.
 * Posted by waypoint loading, task loading, etc.
 */
struct TerrainPrefetchRequestEvent {
  GeoBounds bounds;
};

} // namespace Event
