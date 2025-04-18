// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Terrain/RasterTerrain.hpp"

TerrainHeight
RasterMap::GetHeight([[maybe_unused]] const GeoPoint &location) const noexcept
{
  return TerrainHeight::Invalid();
}

GeoPoint
RasterMap::GroundIntersection([[maybe_unused]] const GeoPoint &origin,
                              [[maybe_unused]] const int h_origin,
                              [[maybe_unused]] const int h_glide,
                              [[maybe_unused]] const GeoPoint &destination,
                              [[maybe_unused]] const int height_floor) const noexcept
{
  return GeoPoint::Invalid();
}

RasterMap::Intersection
RasterMap::FirstIntersection([[maybe_unused]] const GeoPoint &origin,
                             [[maybe_unused]] const int h_origin,
                             [[maybe_unused]] const GeoPoint &destination,
                             [[maybe_unused]] const int h_destination,
                             [[maybe_unused]] const int h_virt,
                             [[maybe_unused]] const int h_ceiling,
                             [[maybe_unused]] const int h_safety) const noexcept
{
  return Intersection::Invalid();
}
