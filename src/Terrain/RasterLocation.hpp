/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Math/Point2D.hpp"
#include "Math/Shift.hpp"

/**
 * A point within a RasterMap.
 */
struct RasterLocation : UnsignedPoint2D {
  using UnsignedPoint2D::UnsignedPoint2D;

  constexpr RasterLocation operator>>(unsigned bits) const {
    return RasterLocation(x >> bits, y >> bits);
  }

  constexpr RasterLocation operator<<(unsigned bits) const {
    return RasterLocation(x << bits, y << bits);
  }

  constexpr RasterLocation RoundingRightShift(unsigned bits) const {
    return RasterLocation(::RoundingRightShift(x, bits),
                          ::RoundingRightShift(y, bits));
  }
};

struct SignedRasterLocation : IntPoint2D {
  using IntPoint2D::IntPoint2D;

  constexpr SignedRasterLocation(RasterLocation other)
    :IntPoint2D(other.x, other.y) {}

  constexpr operator RasterLocation() const {
    return RasterLocation(x, y);
  }

  constexpr SignedRasterLocation operator>>(int bits) const {
    return SignedRasterLocation(x >> bits, y >> bits);
  }

  constexpr SignedRasterLocation operator<<(int bits) const {
    return SignedRasterLocation(x << bits, y << bits);
  }

  constexpr SignedRasterLocation RoundingRightShift(unsigned bits) const {
    return SignedRasterLocation(::RoundingRightShift(x, bits),
                                ::RoundingRightShift(y, bits));
  }
};

#endif
