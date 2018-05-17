/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
RasterMap::GetHeight(const GeoPoint &location) const
{
  return TerrainHeight::Invalid();
}

GeoPoint
RasterMap::Intersection(const GeoPoint& origin,
                        const int h_origin,
                        const int h_glide,
                        const GeoPoint& destination,
                        const int height_floor) const
{
  return GeoPoint(Angle::Zero(), Angle::Zero());
}

bool
RasterMap::FirstIntersection(const GeoPoint &origin, const int h_origin,
                             const GeoPoint &destination, const int h_destination,
                             const int h_virt, const int h_ceiling,
                             const int h_safety,
                             GeoPoint& intx, int &h) const
{
  return false;
}
