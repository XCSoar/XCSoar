/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_TERRAIN_RASTER_LOCATION_HPP
#define XCSOAR_TERRAIN_RASTER_LOCATION_HPP

#include <cstdlib>

/**
 * A point within a RasterMap.
 */
struct RasterLocation {
  unsigned x, y;

  RasterLocation() = default;
  RasterLocation(unsigned _x, unsigned _y):x(_x), y(_y) {}

  bool operator==(const RasterLocation &other) const {
    return x == other.x && y == other.y;
  }

  bool operator!=(const RasterLocation &other) const {
    return !(*this == other);
  }

  RasterLocation operator>>(unsigned bits) const {
    return RasterLocation(x >> bits, y >> bits);
  }

  RasterLocation operator<<(unsigned bits) const {
    return RasterLocation(x << bits, y << bits);
  }

  unsigned manhattan_distance(const RasterLocation &other) const {
    return std::abs((int)x - (int)other.x) +
      std::abs((int)y - (int)other.y);
  }
};

#endif
