// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoBounds.hpp"
#include "Geo/Quadrilateral.hpp"

#include <algorithm>

/**
 * Discrete map LOD from #WindowProjection::GetMapScale().  Cache
 * keys for terrain, topography, and airspace must use this, not the
 * raw scale, so pan/rotate/in-band pinch do not rebuild.
 *
 * Bands 0–3 match #TopographyFile::GetFillThinningLevel; band 4 is
 * overview (same thinning tables as 3, different terrain/airspace
 * policy).
 */
constexpr unsigned MAP_LOD_COUNT = 5;
constexpr unsigned MAP_LOD_FILL_MAX = 3;

constexpr double MAP_LOD_EDGE[] = {
  1200.,
  2500.,
  5000.,
  15000.,
};

static_assert(sizeof(MAP_LOD_EDGE) / sizeof(MAP_LOD_EDGE[0]) + 1 ==
              MAP_LOD_COUNT);

/** Representative GetMapScale() for spacing inside a band. */
constexpr double MAP_LOD_REPRESENTATIVE[] = {
  600.,
  1850.,
  3750.,
  10000.,
  25000.,
};

static_assert(sizeof(MAP_LOD_REPRESENTATIVE) /
              sizeof(MAP_LOD_REPRESENTATIVE[0]) == MAP_LOD_COUNT);

[[gnu::const]]
inline unsigned
MapLod(double map_scale) noexcept
{
  if (map_scale > MAP_LOD_EDGE[3])
    return 4;
  if (map_scale > MAP_LOD_EDGE[2])
    return 3;
  if (map_scale > MAP_LOD_EDGE[1])
    return 2;
  if (map_scale > MAP_LOD_EDGE[0])
    return 1;
  return 0;
}

[[gnu::const]]
inline unsigned
MapLodFillLevel(double map_scale) noexcept
{
  return std::min(MapLod(map_scale), MAP_LOD_FILL_MAX);
}

[[gnu::const]]
inline double
MapLodRepresentativeMapScale(unsigned lod) noexcept
{
  if (lod >= MAP_LOD_COUNT)
    lod = MAP_LOD_COUNT - 1;
  return MAP_LOD_REPRESENTATIVE[lod];
}

/**
 * Scale a live PixelsToAngle() result onto the representative
 * map_scale of #lod so index caches do not key off exact zoom.
 */
[[gnu::const]]
inline double
MapLodSpacingFactor(double map_scale, unsigned lod) noexcept
{
  if (map_scale <= 0)
    return 1;
  return MapLodRepresentativeMapScale(lod) / map_scale;
}

/**
 * True when the current screen quad is still inside a padded vis
 * cache.  Test corners, not the rotated AABB: a twist grows
 * GetScreenBounds() even when the view has not left the pad.
 */
[[gnu::pure]]
inline bool
MapLodQuadInside(const GeoBounds &cached,
                 const GeoQuadrilateral &q) noexcept
{
  return cached.IsValid() && q.IsDefined() &&
    cached.IsInside(q.top_left) &&
    cached.IsInside(q.top_right) &&
    cached.IsInside(q.bottom_left) &&
    cached.IsInside(q.bottom_right);
}
