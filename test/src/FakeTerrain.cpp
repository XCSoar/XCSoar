/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
