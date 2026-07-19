// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/custom/GeoBitmap.hpp"

#include <ctime>

namespace SkySight {

inline constexpr unsigned RECENT_LIVE_TILE_FALLBACK_STEPS = 3;
inline constexpr time_t LIVE_TILE_INTERVAL_SECONDS = 600;

[[nodiscard]] constexpr bool
IsRecentLiveTileTimestamp(time_t candidate, time_t reference) noexcept
{
  return candidate > 0 && candidate <= reference &&
    reference - candidate <
      time_t(RECENT_LIVE_TILE_FALLBACK_STEPS) * LIVE_TILE_INTERVAL_SECONDS;
}

[[nodiscard]] constexpr bool
IsSameTile(const GeoBitmap::TileData &a,
           const GeoBitmap::TileData &b) noexcept
{
  return a.zoom == b.zoom && a.x == b.x && a.y == b.y;
}

[[nodiscard]] constexpr GeoBitmap::TileData
GetTileAncestor(const GeoBitmap::TileData &tile,
                unsigned ancestor_zoom) noexcept
{
  if (ancestor_zoom >= tile.zoom)
    return tile;

  const auto shift = tile.zoom - ancestor_zoom;
  return {
    static_cast<uint16_t>(ancestor_zoom),
    tile.x >> shift,
    tile.y >> shift,
  };
}

/**
 * Live images contain little additional visible detail at the geometrically
 * selected map zoom.  Use its immediate parent to reduce the request count,
 * except when the layer is already at its minimum zoom.
 */
[[nodiscard]] constexpr unsigned
SelectLiveTileZoom(unsigned map_zoom, unsigned minimum_zoom) noexcept
{
  return map_zoom > minimum_zoom ? map_zoom - 1 : minimum_zoom;
}

/**
 * The final live zoom is one below the geometric map zoom.  Let the geometric
 * selection advance one level beyond the provider maximum so that subtraction
 * can still reach the provider's highest supported tile level.
 */
[[nodiscard]] constexpr unsigned
GetLiveTileMapZoomMaximum(unsigned maximum_zoom) noexcept
{
  return maximum_zoom < GeoBitmap::MAX_TILE_ZOOM
    ? maximum_zoom + 1
    : maximum_zoom;
}

/**
 * Choose one acquisition time for the whole viewport.  Coverage entries are
 * ordered newest first in LIVE_TILE_INTERVAL_SECONDS steps.  Prefer the time
 * covering most requested tiles and the newer time when coverage is equal.
 */
template<typename Range>
[[nodiscard]] constexpr time_t
SelectCoherentLiveTileTimestamp(time_t newest, const Range &coverage) noexcept
{
  unsigned best_coverage = 0;
  unsigned best_step = 0;
  unsigned step = 0;

  for (const auto value : coverage) {
    if (value > best_coverage) {
      best_coverage = value;
      best_step = step;
    }
    ++step;
  }

  return best_coverage > 0
    ? newest - time_t(best_step) * LIVE_TILE_INTERVAL_SECONDS
    : 0;
}

} // namespace SkySight
